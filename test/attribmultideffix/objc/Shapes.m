#import <Foundation/Foundation.h>
@interface Circle : NSObject
- (int)area;
@end
@implementation Circle
- (int)area { return 3; }
@end
@interface Square : NSObject
- (int)area;
@end
@implementation Square
- (int)area { return 4; }
@end
int circleArea(Circle* c) { return [c area]; }
int squareArea(Square* s) { return [s area]; }
