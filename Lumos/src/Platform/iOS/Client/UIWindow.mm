#include "UIWindow.h"

#include "Core/OS/Input.h"
#include "Events/Event.h"
#include "Events/ApplicationEvent.h"

#include "../iOSOS.h"

@implementation LumosUIWindow

-( void )layoutSubviews
{
    [ super layoutSubviews ];
    //Lumos::iOSOS::Get()->OnScreenResize(self.bounds.size.width, self.bounds.size.height);
}

-( void )touchesBegan:( NSSet< UITouch* >* )touches withEvent:( UIEvent* )event
{
    for( UITouch* touch in touches )
    {
        CGPoint   point = [ touch locationInView:self ];
        NSInteger index = [ touch.estimationUpdateIndex integerValue ];
        Lumos::iOSOS::Get()->OnScreenPressed(point.x, point.y, (u32)index, true);
    }
}

-( void )touchesMoved:( NSSet< UITouch* >* )touches withEvent:( UIEvent* )event
{
    for( UITouch* touch in touches )
    {
        CGPoint   point = [ touch locationInView:self ];
        NSInteger index = [ touch.estimationUpdateIndex integerValue ];
        Lumos::iOSOS::Get()->OnMouseMovedEvent(point.x, point.y);
    }
}

-( void )touchesEnded:( NSSet< UITouch* >* )touches withEvent:( UIEvent* )event
{
    for( UITouch* touch in touches )
    {
        CGPoint   point = [ touch locationInView:self ];
        NSInteger index = [ touch.estimationUpdateIndex integerValue ];
        Lumos::iOSOS::Get()->OnScreenPressed(point.x, point.y, (u32)index,false);
    }
}

-( void )touchesCancelled:( NSSet< UITouch* >* )touches withEvent:( UIEvent* )event
{
    for( UITouch* touch in touches )
    {
        CGPoint   point = [ touch locationInView:self ];
        NSInteger index = [ touch.estimationUpdateIndex integerValue ];
        Lumos::iOSOS::Get()->OnScreenPressed(point.x, point.y, (u32)index,false);
    }
}

@end
