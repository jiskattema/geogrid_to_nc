/* WRF geogrid to NetCDF convertor
 * 
 * (c) 2013 Jisk Attema, Netherlands eScience Center
 * LICENSE: Apache 2.0
 *
 * Purpose of this program is to provide an independent check on the WRF input files.
 */ 

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <getopt.h>
#include <netcdf.h>

#undef BIG_ENDIAN
#undef LITTLE_ENDIAN

#define BIG_ENDIAN    0
#define LITTLE_ENDIAN 1

#define NC( TST )                                                          \
    do {                                                                   \
        int ncstatus = TST;                                                \
        if( ncstatus == NC_NOERR ) {                                       \
            printf( "[OK] %s\n", #TST );                                   \
        } else {                                                           \
            printf( "[XX] %s: %s\n", #TST, nc_strerror(ncstatus) );        \
        }                                                                  \
    } while(0)


void
usage() {
    printf(" Usage: write_geog\n" );
    printf("    --input/-i         inputfile (geogrid)\n" );
    printf("    --output/-o        outputfile (netcdf)\n" );
    printf("    --nx/-x            Grid size NX\n" );
    printf("    --ny/-y            Grid size NY\n" );
    printf("    --nz/-z            Grid size NZ\n" );
    printf("    --wsize/-w         Word size\n" );
    printf("    --scale/-s         Scale factor\n" );
    printf("    --signed/-m        Signed data (default unsigned)\n" );
    printf("    --littleendian/-l  Endianness (default big endian)\n" );
    printf("    --verbose\n" );
    printf("\nThis program converts a WRF geogrid file (which is actually an ENVI file) to NetCDF4.\n" );
    printf("The WRF 'index' file is not parsed, instead all settings should be provided via the commandline options.\n" );
    exit(-1);
};


void
print_statistics(float *data, int nx, int ny, int nz) {
    int i,j,k;

    double min, max, avg;

    for(k=0; k<nz; k++) {
        min = 0.0;
        max = 0.0;
        avg = 0.0;
        for(j=0; j<ny; j++) {
            for(i=0; i<nx; i++) {
                float d = data[k * nx * ny + j * nx + i];
                if( d < min ) min = d;
                if( d > max ) max = d;
                avg += d;
            }
        }
        printf( "%i %lf %lf %lf\n", k+1, avg / (nx * ny), min, max );
    }
}

int
main( int argc, char **argv) {

    char inputfile[256] = "", outputfile[256] = "";
    int nx = 1, ny = 1, nz = 1, wsize = 4, issigned = 0, verbose = 0, endianness = BIG_ENDIAN;
    float scale = 1.0;
     
    static struct option long_options[] = {

            /* These options don't set a flag.
            We distinguish them by their indices. */
            {"input",        required_argument, 0, 'i'},
            {"output",       required_argument, 0, 'o'},
            {"nx",           required_argument, 0, 'x'},
            {"ny",           required_argument, 0, 'y'},
            {"nz",           required_argument, 0, 'z'},
            {"wsize",        required_argument, 0, 'w'},
            {"scale",        required_argument, 0, 's'},
            {"signed",       no_argument,       0, 'm'},
            {"verbose",      no_argument,       0, 'v'},
            {"littleendian", no_argument,       0, 'l'},
            {"help",         no_argument,       0, '?'},
            {0, 0, 0, 0}
    };

    int c;
    while(1) {
        c = getopt_long (argc, argv, "i:o:x:y:z:w:s:mlv", long_options, NULL);

        /* Detect the end of the options. */
        if (c == -1) break;

        switch (c) {
            case 0:
                /* If this option set a flag, do nothing else now. */
                break;

            case 'x':
                nx = atoi( optarg );
                break;

            case 'y':
                ny = atoi( optarg );
                break;
            
            case 'z':
                nz = atoi( optarg );
                break;

            case 'w':
                wsize = atoi( optarg );
                break;

            case 's':
                scale = atof( optarg );
                break;

            case 'i':
                strncpy( inputfile, optarg, 256 );
                break;

            case 'm':
                issigned = 1;
                break;

            case 'l':
                endianness = LITTLE_ENDIAN;
                break;

            case 'o':
                strncpy( outputfile, optarg, 256 );
                break;

            case 'v':
                verbose = 1;
                break;

            case '?':
            default:
                usage();
        }
    }

    if( inputfile[0] == '\0' || outputfile[0] == '\0' ) 
        usage();

    if( verbose ) {
        printf( "Input file:\t\t%s\n", inputfile );
        printf( "output file:\t\t%s\n", outputfile );
        printf( "Grid NX:\t\t%i\n", nx );
        printf( "Grid NY:\t\t%i\n", ny );
        printf( "Grid NZ:\t\t%i\n", nz );
        printf( "Word size:\t\t%i\n", wsize );
        printf( "Scale factor:\t\t%f\n", scale );
        printf( "Signed:\t\t\t%s\n", issigned ? "yes" : "no" );
        printf( "Endianness:\t\t%s\n", endianness == LITTLE_ENDIAN ? "little" : "big" );
    }

    float *data = malloc( nx * ny * nz * sizeof(float) );

    int status = 0;
    int filenamelen = strlen( inputfile );
    read_geogrid(  inputfile,                /* The name of the file to read from */
                   &filenamelen,             /* The length of the filename */
                   data,                     /* The array to be filled */
                   &nx,                      /* x-dimension of the array */
                   &ny,                      /* y-dimension of the array */
                   &nz,                      /* z-dimension of the array */
                   &issigned,                /* 0=unsigned data, 1=signed data */
                   &endianness,              /* 0=big endian, 1=little endian */
                   &scale,                   /* value to multiply array elements by before truncation to integers */
                   &wsize,                   /* number of bytes to use for each array element */
                   &status );

    printf( "Read geogrid status: %i\n", status );

    int ncid, dimids[3], varid;

    NC( nc_create( outputfile, NC_WRITE | NC_NETCDF4, &ncid ) );
    if( nz > 1 ) {
        NC( nc_def_dim( ncid, "x", nx, &dimids[2]) );
        NC( nc_def_dim( ncid, "y", ny, &dimids[1]) );
        NC( nc_def_dim( ncid, "z", nz, &dimids[0]) );
        NC( nc_def_var( ncid, "var", NC_FLOAT, 3, dimids, &varid ) );
    }
    else {
        NC( nc_def_dim( ncid, "x", nx, &dimids[1]) );
        NC( nc_def_dim( ncid, "y", ny, &dimids[0]) );
        NC( nc_def_var( ncid, "var", NC_FLOAT, 2, dimids, &varid ) );
    }
    NC( nc_def_var_deflate( ncid, varid, NC_NOSHUFFLE, 1, 4 ) );
    NC( nc_enddef( ncid ) );

    NC( nc_put_var( ncid, varid, data ) );

    NC( nc_close( ncid ) );

    print_statistics(data,nx,ny,nz);

    // clean up
    free( data );
    exit (0);
}
