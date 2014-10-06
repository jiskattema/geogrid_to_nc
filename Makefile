write_geog: geogrid_to_nc.c read_geogrid.c
	gcc -o geogrid_to_nc geogrid_to_nc.c `pkg-config --cflags --libs netcdf` read_geogrid.c

clean:
	-rm geogrid_to_nc
