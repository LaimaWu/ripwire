class Circle { area() { return 3; } }
class Square { area() { return 4; } }
function circleArea() { const c = new Circle(); return c.area(); }
function squareArea() { const s = new Square(); return s.area(); }
module.exports = { circleArea, squareArea };
