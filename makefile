MODULES = quadtree
EXTENSION = quadtree
DATA = quadtree--1.0.sql
REGRESS = quadtree_test
PGXS := $(shell pg_config --pgxs)
include $(PGXS)