export class Circle {
  area(): number { return 3; }
}
export class Square {
  area(): number { return 4; }
}
export function circleArea(): number {
  const c = new Circle();
  return c.area();
}
export function squareArea(): number {
  const s = new Square();
  return s.area();
}
export function bothAreas(): number {
  const c = new Circle();
  const s = new Square();
  return c.area() + s.area();
}
