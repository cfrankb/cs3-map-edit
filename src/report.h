#pragma once

class CMapFile;
class QString;
class CMap;

bool generateReport(CMapFile & mf, const QString & filename);
void generateScreenshot(const QString &filename, CMap *map, const int maxRow=16, const int maxCols=16);
