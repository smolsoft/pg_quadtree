#include "postgres.h"
#include "fmgr.h"
#include "funcapi.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

#ifdef PG_MODULE_MAGIC
PG_MODULE_MAGIC;
#endif

extern text *cstring_to_text(const char *s); 
extern char *text_to_cstring(const text *t); 

#define TILE_SIZE 	256

#define MAX_TREE_LEVEL	31

#define NEIGHBOR_TOP 	0
#define NEIGHBOR_RIGHT 	1
#define NEIGHBOR_BOTTOM 2
#define NEIGHBOR_LEFT 	3


typedef struct linkedList
{
	char value[MAX_TREE_LEVEL + 1];
	struct linkedList *next;
} linkedList;


char *calcQuadTree(double longitude, double latitude, int max_level, char* refQuad);
char *calcQuadTreeNoTile(double longitude, double latitude, int max_level, char* refQuad);

char * newstrcat(char *src, char* append);


linkedList *addValueAppend (linkedList *last, char *value, char *append);
linkedList *getQuadsAlongSide(linkedList *list, char *srcQuad, int sideType, int deepLevel);

char *calcNeighborQuad(char *srcQuad, int32 neighborType);




/* PG FUNC: QuadTree
Parameters:
	longitude double precision, -- longitude (X coord)
	latitude double precision, -- latitude (Y coord)
    max_level int -- depth of calculated quad-tree 
Returns:
	text -- quad-tree index string with max_level length
*/
PG_FUNCTION_INFO_V1( QuadTree);
Datum QuadTree( PG_FUNCTION_ARGS )
{
	PG_RETURN_TEXT_P(cstring_to_text(calcQuadTree(
		PG_GETARG_FLOAT8(0),
		PG_GETARG_FLOAT8(1),
		PG_GETARG_INT32(2),
		0)));		
}

PG_FUNCTION_INFO_V1( QuadTreeNo);
Datum QuadTreeNo( PG_FUNCTION_ARGS )
{
	PG_RETURN_TEXT_P(cstring_to_text(calcQuadTreeNoTile(
		PG_GETARG_FLOAT8(0),
		PG_GETARG_FLOAT8(1),
		PG_GETARG_INT32(2),
		0)));		
}


/* PG FUNC: CalcQuadTreeRect
Parameters:
	leftX, -- longitude of top left rect's corner
	topY, -- latitude of top left rect's corner
	rightX, -- 
	bottomY
Returns:
	text -- deepest possible quad-tree index for whole rectangle
*/
PG_FUNCTION_INFO_V1( QuadTreeRect );
Datum QuadTreeRect( PG_FUNCTION_ARGS )
{
	float8 leftX = PG_GETARG_FLOAT8(0);
	float8 topY = PG_GETARG_FLOAT8(1);
	float8 rightX = PG_GETARG_FLOAT8(2);
	float8 bottomY = PG_GETARG_FLOAT8(3);
	int max_level = PG_GETARG_INT32(4);

	char *result = calcQuadTree(leftX, topY, max_level, 0);
	result = calcQuadTree(rightX, topY, max_level, result);
	result = calcQuadTree(rightX, bottomY, max_level, result);
	result = calcQuadTree(leftX, bottomY, max_level, result);
	
	PG_RETURN_TEXT_P(cstring_to_text(result));	
}

char *calcQuadTree(double longitude, double latitude, int max_level, char* refQuad)
{
	double x, y, sinLat;
	int tiX, tiY;
	char *result;

	if (max_level > MAX_TREE_LEVEL) max_level = MAX_TREE_LEVEL;
	if (max_level < 1) max_level = 1;
	
	x = (longitude + 180) / 360 * TILE_SIZE;
	sinLat = sin(latitude * M_PI / 180);
	y = (0.5 - log((1 + sinLat) / (1 - sinLat)) / (4.0 * M_PI)) * TILE_SIZE;

	tiX = floor(x * pow(2, max_level) / TILE_SIZE);
	tiY = floor(y * pow(2, max_level) / TILE_SIZE);

elog(NOTICE, "TILE: tiX = %i, tiY = %i", tiX, tiY);

	result = palloc(max_level * sizeof(char) + 1);
	memset(result, '\0', max_level * sizeof(char) + 1);
	
	for(int i = max_level; i > 0; i--)
	{
		int pow = 1 << (i - 1);
		int cell = 0;
		if (tiX & pow) cell += 1;
		if (tiY & pow) cell += 2;
		
		// on diff with refQuad - stop and return current result
		if (refQuad &&
			(strlen(refQuad) <= max_level - i || refQuad[max_level - i] != '0' + cell))
			return result;
			
		result[max_level - i] = '0' + cell;
	}

	return result;
}

double latMid(double y1, double y2);

PG_FUNCTION_INFO_V1( QuadCoor );
Datum QuadCoor( PG_FUNCTION_ARGS )
{
	double x0 = -180, x1 = 180;
	double y0 = -90, y1 = 90;
	char *quad = VARDATA(PG_GETARG_TEXT_P(0));
	char *result = palloc(100 * sizeof(char));

	for(int i=0; i < strlen(quad); i++)
	{
		double xmid = (x0 + x1) / 2;
		double ymid = latMid(y0, y1);
		switch (quad[i])
		{
			case '0':
				x1 = xmid;
				y0 = ymid;
				break;

			case '1':
				x0 = xmid;
				y0 = ymid;
				break;

			case '2':
				x1 = xmid;
				y1 = ymid;
				break;

			case '3':
				x0 = xmid;
				y1 = ymid;
				break;
		}
	}

    sprintf(result, "x=[%f, %f] y=[%f, %f]", x0, x1, y0, y1);

	PG_RETURN_TEXT_P(cstring_to_text(result));
}

double latMid(double y1, double y2)
{
	double sin1 = sin(y1 * M_PI / 180);
	double sin2 = sin(y2 * M_PI / 180);
	double result = asin((sin2 + sin1) / 2) * 180 / M_PI;
	elog(NOTICE, "latMid(%f, %f) = %f", y1 , y2, result);
	return result;
}

char *calcQuadTreeNoTile(double longitude, double latitude, int max_level, char* refQuad)
{
	double x, y, sinLat;
	int tiX, tiY;
	char *result;

	if (max_level > MAX_TREE_LEVEL) max_level = MAX_TREE_LEVEL;
	if (max_level < 1) max_level = 1;
	
	x = (longitude + 180) / 360;
	sinLat = sin(latitude * M_PI / 180);
	y = (0.5 - log((1 + sinLat) / (1 - sinLat)) / (4.0 * M_PI));

	tiX = floor(x * pow(2, max_level));
	tiY = floor(y * pow(2, max_level));

elog(NOTICE, "NO_TILE: tiX = %i, tiY = %i", tiX, tiY);

	result = palloc(max_level * sizeof(char) + 1);
	memset(result, '\0', max_level * sizeof(char) + 1);
	
	for(int i = max_level; i > 0; i--)
	{
		int pow = 1 << (i - 1);
		int cell = 0;
		if (tiX & pow) cell += 1;
		if (tiY & pow) cell += 2;
		
		// on diff with refQuad - stop and return current result
		if (refQuad &&
			(strlen(refQuad) <= max_level - i || refQuad[max_level - i] != '0' + cell))
			return result;
			
		result[max_level - i] = '0' + cell;
	}

	return result;
}



/* PG FUNC: QuadsAlongBorders
Параметры:
	_srcQuad text,
    _deep_level int
Возвращает:
	set of text --список quad-индексов вокруг _srcQuad c заданной глубиной _deep_level (0 - по 1 на сторону, 1 - по 2 на сторону, 3- по 4 на сторону 4 - по 16 на сторону и т.д)
*/
PG_FUNCTION_INFO_V1( QuadsAlongBorders);
Datum QuadsAlongBorders( PG_FUNCTION_ARGS )
{
	FuncCallContext  *funcctx;
	linkedList *cur;
	
	if (SRF_IS_FIRSTCALL())
    {	
        MemoryContext oldcontext;
		linkedList fakeRoot, *last;

		// сохранить и проверить исходные параметры
		char *srcQuad = VARDATA(PG_GETARG_TEXT_P(0));
		int32 deepLevel = PG_GETARG_INT32(1);

		if (strlen(srcQuad) < 1 || strlen(srcQuad) > MAX_TREE_LEVEL - deepLevel)
			elog(ERROR, "QuadsAlongBorders: srcQuad parameter length MUST be in range from 1 to %i (MAX_TREE_LEVEL - deepLevel)", MAX_TREE_LEVEL - deepLevel);
		
		if (deepLevel < 0 || deepLevel > 10) 
			elog(ERROR, "QuadsAlongBorders: deepLevel parameter MUST be in range from 0 to 10");


        funcctx = SRF_FIRSTCALL_INIT();
        oldcontext = MemoryContextSwitchTo(funcctx->multi_call_memory_ctx);

	
		// собираем список квадов
		if (deepLevel == 0) {
			last = addValueAppend(&fakeRoot, calcNeighborQuad(srcQuad, NEIGHBOR_TOP), "");
			last = addValueAppend(last, calcNeighborQuad(srcQuad, NEIGHBOR_BOTTOM), "");
			last = addValueAppend(last, calcNeighborQuad(srcQuad, NEIGHBOR_LEFT), "");
			last = addValueAppend(last, calcNeighborQuad(srcQuad, NEIGHBOR_RIGHT), "");
		} else {
			last = getQuadsAlongSide(&fakeRoot, calcNeighborQuad(srcQuad, NEIGHBOR_TOP), NEIGHBOR_BOTTOM, deepLevel);
			last = getQuadsAlongSide(last, calcNeighborQuad(srcQuad, NEIGHBOR_BOTTOM), NEIGHBOR_TOP, deepLevel);
			last = getQuadsAlongSide(last, calcNeighborQuad(srcQuad, NEIGHBOR_LEFT), NEIGHBOR_RIGHT, deepLevel);
			last = getQuadsAlongSide(last, calcNeighborQuad(srcQuad, NEIGHBOR_RIGHT), NEIGHBOR_LEFT, deepLevel);
		}
		
		funcctx->user_fctx = fakeRoot.next;// fake root is empty, start with it next
		
        MemoryContextSwitchTo(oldcontext);
    }
	
	funcctx = SRF_PERCALL_SETUP(); // получаем контекст выполнения
	
	cur = (linkedList *)funcctx->user_fctx;
	
	if (cur) 
	{
		funcctx->user_fctx = cur->next;// move next
		SRF_RETURN_NEXT(funcctx, (Datum)cstring_to_text(cur->value));
	} else {
		SRF_RETURN_DONE(funcctx);
	}

}

/* рекурсивно рассчитывает quad-индексы со стороны sideType квада srcQuad */
linkedList *getQuadsAlongSide(linkedList *list, char *srcQuad, int sideType, int deepLevel)
{
	switch (sideType)
	{
		case NEIGHBOR_TOP:
			if (deepLevel > 1) {
				list = getQuadsAlongSide(list, newstrcat(srcQuad, "0"), sideType, deepLevel - 1);
				list = getQuadsAlongSide(list, newstrcat(srcQuad, "1"), sideType, deepLevel - 1);
			} else {
				list = addValueAppend(list, srcQuad, "0");
				list = addValueAppend(list, srcQuad, "1");
			}
			break;

		case NEIGHBOR_BOTTOM:
			if (deepLevel > 1) {
				list = getQuadsAlongSide(list, newstrcat(srcQuad, "2"), sideType, deepLevel - 1);
				list = getQuadsAlongSide(list, newstrcat(srcQuad, "3"), sideType, deepLevel - 1);
			} else {
				list = addValueAppend(list, srcQuad, "2");
				list = addValueAppend(list, srcQuad, "3");
			}
			break;
			
		case NEIGHBOR_LEFT:
			if (deepLevel > 1) {
				list = getQuadsAlongSide(list, newstrcat(srcQuad, "0"), sideType, deepLevel - 1);
				list = getQuadsAlongSide(list, newstrcat(srcQuad, "2"), sideType, deepLevel - 1);
			} else {
				list = addValueAppend(list, srcQuad, "0");
				list = addValueAppend(list, srcQuad, "2");
			}
			break;
			
		case NEIGHBOR_RIGHT:
			if (deepLevel > 1) {
				list = getQuadsAlongSide(list, newstrcat(srcQuad, "1"), sideType, deepLevel - 1);
				list = getQuadsAlongSide(list, newstrcat(srcQuad, "3"), sideType, deepLevel - 1);
			} else {
				list = addValueAppend(list, srcQuad, "1");
				list = addValueAppend(list, srcQuad, "3");
			}
			break;
	}
			
	return list;
}

/*	Рассчитывает код соседнего квада с указанной стороны
Параметры:
	srcQuad - Исходный quadCode
	neighborType - тип соседа : 0-top, 1-right, 2-bottom, 3-left
Возвращает:
	строку квад индекса соседа
*/
char *calcNeighborQuad(char *srcQuad, int32 neighborType)
{
	char *resQuad;
	int skipOther;
		
	if (neighborType < 0 || 3 < neighborType)
		elog(ERROR, "calcNeighborQuad: parameter 'neighborType' MUST be one of: 0 (top), 1 (right), 2 (bottom), 3 (left)");
	
	if (strlen(srcQuad) < 1 || strlen(srcQuad) > MAX_TREE_LEVEL)
		elog(ERROR, "calcNeighborQuad: parameter 'srcQuad' length MUST be in range from 1 to %i", MAX_TREE_LEVEL);
	
    resQuad = palloc(MAX_TREE_LEVEL + 1);
	memset(resQuad, '\0', MAX_TREE_LEVEL + 1);
	
	skipOther = 0;

	for (int i = strlen(srcQuad) - 1; i >= 0; i--)
	{
		if (skipOther) {
			resQuad[i] = srcQuad[i];
		} else {
			int currentNumber = srcQuad[i] - '0';
			int newNumber = 0;

			switch (neighborType)
			{
				case NEIGHBOR_TOP:
					if (currentNumber >= 2) { //one of bottom quads
						newNumber = currentNumber - 2;
						skipOther = true;
					} else {
						newNumber = currentNumber + 2;
					}
					break;
				
				case NEIGHBOR_BOTTOM:
					if (currentNumber >= 2)	{// one of bottom quads
						newNumber = currentNumber - 2;
					} else {
						newNumber = currentNumber + 2;
						skipOther = true;
					}
					break;

				case NEIGHBOR_RIGHT:
					if (currentNumber == 0 || currentNumber == 2) {// one of left quads
						newNumber = currentNumber + 1;
						skipOther = true;
					} else {
						newNumber = currentNumber - 1;
					}
					break;
					
				case NEIGHBOR_LEFT:
					if (currentNumber == 0 || currentNumber == 2)// one of left quads
					{
						newNumber = currentNumber + 1; 
					} else {
						newNumber = currentNumber - 1; 
						skipOther = true;
					}
					break;
			}
			resQuad[i] = '0' + newNumber;
		}
	}

	return resQuad;
}



/* добавляет новый элемент в список со копией объединенной строки value + append
   возвращает добавленный элемент */
linkedList *addValueAppend (linkedList *last, char *value, char *append)
{
	linkedList *new = (linkedList *)palloc(sizeof(linkedList));
	memset(new->value, '\0', MAX_TREE_LEVEL + 1);
	new->next = 0;

	memcpy(new->value, value, MAX_TREE_LEVEL); 
	strcat(new->value, append);

	if (last)
		last->next = new;
	return new;
}

/* объединяет строки с сохранением в новой строке
   возвращает новую строку, равную src + append */
char * newstrcat(char *src, char* append)
{
	char * result;
	int srcLen, resultLen;
	
	srcLen = strlen(src);
	resultLen = srcLen + strlen(append) + 1;
	
	result = palloc(resultLen);
	memset(result, '\0', resultLen);
	memcpy(result, src, srcLen);
	strcat(result, append);
	
	return result;
}




