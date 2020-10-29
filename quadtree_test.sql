SELECT CalcQuadTree(56, 38, 20);

SELECT clrfn_Geo_CalcQuadTreeRect(55.554, 37.554, 55.556, 37.556, 20);

SELECT clrfn_Geo_CalcQuadTreeRect(55, 37, 56, 38, 20);

SELECT * FROM GetQuadsAlongBorders(CalcQuadTree(56, 38, 20), 1);
