#include "UIWindow.h"

#include "Core/OS/Input.h"
#include "Events/Event.h"
#include "Events/ApplicationEvent.h"

@implementation LumosUIWindow

-( void )layoutSubviews
{
    [ super layoutSubviews ];

//    WindowResizedEvent e;
//    e.width  = self.bounds.size.width;
//    e.height = self.bounds.size.height;
}

-( void )touchesBegan:( NSSet< UITouch* >* )touches withEvent:( UIEvent* )event
{
    for( UITouch* touch in touches )
    {
        CGPoint   point = [ touch locationInView:self ];
        NSInteger index = [ touch.estimationUpdateIndex integerValue ];
    }
}

-( void )touchesMoved:( NSSet< UITouch* >* )touches withEvent:( UIEvent* )event
{
    for( UITouch* touch in touches )
    {
        CGPoint   point = [ touch locationInView:self ];
        NSInteger index = [ touch.estimationUpdateIndex integerValue ];
    }
}

-( void )touchesEnded:( NSSet< UITouch* >* )touches withEvent:( UIEvent* )event
{
    for( UITouch* touch in touches )
    {
        CGPoint   point = [ touch locationInView:self ];
        NSInteger index = [ touch.estimationUpdateIndex integerValue ];
    }
}

-( void )touchesCancelled:( NSSet< UITouch* >* )touches withEvent:( UIEvent* )event
{
    for( UITouch* touch in touches )
    {
        CGPoint   point = [ touch locationInView:self ];
        NSInteger index = [ touch.estimationUpdateIndex integerValue ];
    }
}

@end
