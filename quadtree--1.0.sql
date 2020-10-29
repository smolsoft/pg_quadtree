/* pg_aa/quadtree_1.0.sql */
-- complain if script is sourced in psql, rather than via CREATE EXTENSION
\echo Use ”CREATE EXTENSION quadtree” to load this file. \quit
--
-- PostgreSQL code for pg_aa.
--
CREATE OR REPLACE FUNCTION QuadTree(double precision, double precision, integer)
RETURNS TEXT
AS 'MODULE_PATHNAME', 'QuadTree' LANGUAGE C STRICT;

CREATE OR REPLACE FUNCTION QuadTreeRect(double precision, double precision, double precision, double precision, integer)
RETURNS TEXT
AS 'MODULE_PATHNAME', 'QuadTreeRect' LANGUAGE C STRICT;

CREATE OR REPLACE FUNCTION QuadsAlongBorders(text, integer)
RETURNS SETOF TEXT
AS 'MODULE_PATHNAME', 'QuadsAlongBorders' LANGUAGE C STRICT;