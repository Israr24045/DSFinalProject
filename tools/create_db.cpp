#include <iostream>
#include "database.h"

int main() {
    Database db("data/canvas.omni");
    // Force initialize (simulate fresh start)
    db.initialize();
    bool ok = db.save();
    std::cout << "save() returned: " << ok << std::endl;
    return ok ? 0 : 1;
}
