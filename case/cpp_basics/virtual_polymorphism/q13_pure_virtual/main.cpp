#include <iostream>

struct IShape {
    virtual ~IShape() = default;
    virtual double area() const = 0;
};

struct Square : IShape {
    explicit Square(double s) : side(s) {}
    double area() const override { return side * side; }
    double side;
};

int main() {
    Square s(3.0);
    std::cout << s.area() << "\n";
}
