#pragma once
#include <qgsmarkersymbol.h>
#include <qgslinesymbol.h>
#include <qgsfillsymbol.h>
#include <qgssymbol.h>

class CustomMarkerSymbol {

public:
    CustomMarkerSymbol(QgsMarkerSymbol* symbol, QString name) {
        this->symbol = symbol;
        this->name = name;
    }
    QgsMarkerSymbol* getSymbol() {
        return symbol;
    }
    QString getName() {
        return name;
    }
private:
    QgsMarkerSymbol* symbol;
    QString name;
};

class CustomLineSymbol {

public:
    CustomLineSymbol(QgsLineSymbol* symbol, QString name) {
        this->symbol = symbol;
        this->name = name;
    }
    QgsLineSymbol* getSymbol() {
        return symbol;
    }
    QString getName() {
        return name;
    }
private:
    QgsLineSymbol* symbol;
    QString name;
};

class CustomFillSymbol {

public:
    CustomFillSymbol(QgsFillSymbol* symbol, QString name) {
        this->symbol = symbol;
        this->name = name;
    }
    QgsFillSymbol* getSymbol() {
        return symbol;
    }
    QString getName() {
        return name;
    }
private:
    QgsFillSymbol* symbol;
    QString name;
};