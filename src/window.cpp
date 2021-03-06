#include "window.h"

Window::Window(int w, int h, std::string t) {
    width = w; height = h; title = t;
    new(&app) sf::RenderWindow(sf::VideoMode(width, height), title,
                         sf::Style::Titlebar | sf::Style::Close);
    tileList.loadFromFile("tilelist.png");
    font.loadFromFile("font.ttf");

    isStartSet = 0;
    isFinishSet = 0;
    isProcedCells = 0;
    previousCell = NULL;

    size = 32;
    cols = width / size;
    rows = height / size;
    cell = new Cell**[cols];
    for(int i=0; i<cols; i++) {
        cell[i] = new Cell*[rows];
    }
    for(int i = 0; i<cols; i++) {
        for(int j = 0; j<rows; j++) {
            cell[i][j] = new Cell(sf::Vector2i(i*size,j*size),&tileList,&app,&font);
        }
    }
}

int Window::mainLoop() {
    sf::Event event;
    sf::Clock theClock;
    float startTime = theClock.getElapsedTime().asSeconds();

    while (app.isOpen()) {
        while (app.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                app.close();
            if (event.type == sf::Event::KeyPressed) {
                onKeyPressed(event.key.code);
            }
            if (event.type == sf::Event::MouseButtonReleased)
                onMouseRelease(event.mouseButton.button,
                               event.mouseButton.x, event.mouseButton.y);
            if (sf::Mouse::isButtonPressed(sf::Mouse::Right)) {
                sf::Vector2i pos = sf::Mouse::getPosition(app);
                onMouse(pos.x,pos.y);
            }
        }

        float currentTime = theClock.getElapsedTime().asSeconds();
        if (currentTime - startTime >= 0.1) {
            if (isProcedCells == 1) {
                isProcedCells = findNextCell(1);
            }
            startTime = currentTime;
        }

        app.clear();
        drawSprites(1);
        drawColsRows();
        app.display();
    }

    return 0;
}

int Window::startProc(int border) {
    if (!isProcedCells) {
        for(int i = border; i<cols; i++) {
            for(int j = border; j<rows; j++) {
                if (cell[i][j]->getType() == "finish")
                    cell[i][j]->setAmount(0);
            }
        }
        isProcedCells = procCells(1);
    }
    return 0;
}

int Window::checkPlace(Cell *p,int takenAmount) {
    int isProced = 1;
    if (p->checkAmount() == -1) {
        if (p->setAmount(takenAmount+1) == 0)
            isProced = 0;
    }
    return isProced;
}

int Window::procCells(int border) {
    int isProced = 1;
    do {
        isProced = 1;
        for(int i = border; i<cols; i++) {
            for(int j = border; j<rows; j++) {
                int takenAmount = cell[i][j]->checkAmount();
                if (takenAmount != -1) {
                    int returned;
                    if (i+1 < cols) {
                        returned = checkPlace(cell[i+1][j],takenAmount);
                        if (!returned) isProced = 0;
                    }
                    if (j+1 < rows) {
                        returned = checkPlace(cell[i][j+1],takenAmount);
                        if (!returned) isProced = 0;
                    }
                    returned = checkPlace(cell[i-1][j],takenAmount);
                    if (!returned) isProced = 0;

                    returned = checkPlace(cell[i][j-1],takenAmount);
                    if (!returned) isProced = 0;
                }
            }
        }
    } while (!isProced);
    return 1;
}

Cell* Window::compareCells(int min, Cell *mp, Cell *p) {
    if (p->getType() == "checked" || p->getType() == "finish") {
        int cur = p->checkAmount();
        if (cur<min) {
            return p;
        }
    }
    return mp;
}

int Window::findNextCell(int border) {
    int isProced = 1;
    for(int i = border; i<cols; i++) {
        for(int j = border; j<rows; j++) {

            if ((cell[i][j]->getType() == "start") ||
                (cell[i][j]->getType() == "current")) {

                if (previousCell != NULL) {
                    previousCell->change("steped");
                }

                Cell* minPointer;
                minPointer = cell[i][j];
                int minAmount = cell[i][j]->checkAmount();

                if (i+1 < cols) {
                    minPointer = compareCells(minAmount, minPointer, cell[i+1][j]);
                    minAmount = minPointer->checkAmount();
                }
                if (j+1 < rows) {
                    minPointer = compareCells(minAmount, minPointer, cell[i][j+1]);
                    minAmount = minPointer->checkAmount();
                }
                minPointer = compareCells(minAmount, minPointer, cell[i][j-1]);
                minAmount = minPointer->checkAmount();
                minPointer = compareCells(minAmount, minPointer, cell[i-1][j]);

                if (minPointer->getType() == "finish") {
                    isProced = 2;
                    minPointer->change("start");
                    return isProced;
                }
                if (minPointer->getType() == "checked") {
                    minPointer->change("current");
                    previousCell = minPointer;
                }
                if (cell[i][j]->getType() == "start")
                    cell[i][j]->change("steped");

                return isProced;
            }
        }
    }
    return isProced;
}

int Window::onMouseRelease(sf::Mouse::Button button, int x, int y) {
    if (x/size >= cols || y/size >= rows ||
        x < 0 || y < 0) return 1;
    Cell *pointer;
    if ((pointer = cell[x/size][y/size]) != NULL) {
        if (button == sf::Mouse::Left) {
            if (isStartSet == 0) {
                isStartSet = 1;
                pointer->change("start");
            } else {
                if (isFinishSet == 0) {
                    isFinishSet = 1;
                    pointer->change("finish");
                }
            }
        }
    }
    return 0;
}

int Window::onKeyPressed(sf::Keyboard::Key key) {
    if (key == sf::Keyboard::Escape)
        app.close();
    if (key == sf::Keyboard::Space)
        startProc(1);
    return 0;
}

int Window::onMouse(int x, int y) {
    Cell *pointer;
    if (x/size >= cols || y/size >= rows ||
        x < 0 || y < 0) return 1;
    if ((pointer = cell[x/size][y/size]) != NULL) {
        std::string deletedType;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift))
            deletedType = pointer->change("free");
        else
            deletedType = pointer->change("wall");

        if (deletedType == "finish") isFinishSet = 0;
        if (deletedType == "start") isStartSet = 0;
    }
    return 0;
}

int Window::drawText(std::string text, int x, int y, int size) {
    sf::Text outputText;
    outputText.setFont(font);
    outputText.setPosition(x,y);
    outputText.setString(text);
    outputText.setCharacterSize(size);
    outputText.setColor(sf::Color(255, 255, 255));
    app.draw(outputText);
    return 0;
}

int Window::drawSprites(int border) {
    for(int i = border; i<cols; i++) {
        for(int j = border; j<rows; j++) {
            cell[i][j]->step();
        }
    }
    return 0;
}

int Window::drawColsRows() {
    for(int i=0; i<rows; i++) {
        std::string str = "";
        str += toString(i);
        drawText(str,10,i*32+5,16);
    }

    for(int i=0; i<cols; i++) {
        std::string str = "";
        str += toString(i);
        drawText(str,i*32+8,10,16);
    }
    return 0;
}

std::string Window::toString(int number) {
    std::string str = "";
    for(str = ""; number; number /= 10)
        str += (char)('0' + number%10);
    reverse(str.begin(), str.end());
    return str;
}

Window::~Window() {

}
