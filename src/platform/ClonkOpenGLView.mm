/*
 * OpenClonk, http://www.openclonk.org
 *
 * Portions might be copyrighted by other authors who have contributed
 * to OpenClonk.
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 * See isc_license.txt for full license and disclaimer.
 *
 * "Clonk" is a registered trademark of Matthes Bender.
 * See clonk_trademark_license.txt for full license.
 */

#include <C4Include.h>
#include <C4GraphicsSystem.h>
#include <C4MouseControl.h>
#include <C4GUI.h>
#include <C4Game.h>
#include <C4Viewport.h>
#include <C4ViewportWindow.h>
#include <C4Console.h>
#include <C4Fullscreen.h>
#include <C4PlayerList.h>
#include <C4Gui.h>
#include <C4Landscape.h>

#include <C4DrawGL.h>

#import "ClonkOpenGLView.h"
#import "ClonkWindowController.h"
#import "ClonkMainMenuActions.h"

#ifdef USE_COCOA

@implementation ClonkOpenGLView

@synthesize context;

- (BOOL) isOpaque {return YES;}

- (void) dealloc
{
	[context release];
	[super dealloc];
}

- (id) initWithFrame:(NSRect)frameRect
{
	self = [super initWithFrame:frameRect];
	if (self != nil) {
		[[NSNotificationCenter defaultCenter]
			addObserver:self
			selector:@selector(_surfaceNeedsUpdate:)
			name:NSViewGlobalFrameDidChangeNotification
			object:self];
	}
	return self;
}

- (void) awakeFromNib
{
	[self enableEvents];
}

- (void) enableEvents
{
	[[self window] makeFirstResponder:self];
	[[self window] setAcceptsMouseMovedEvents:YES];
	
	[self registerForDraggedTypes:[NSArray arrayWithObjects:NSFilenamesPboardType, nil]];
}

- (BOOL) acceptsFirstResponder {return YES;}

- (void) resetCursorRects
{
    [super resetCursorRects];
	if ([self shouldHideMouseCursor])
	{
		static NSCursor* cursor;
		if (!cursor)
		{
			cursor = [[NSCursor alloc] initWithImage:[[NSImage alloc] initWithSize:NSMakeSize(1, 1)] hotSpot:NSMakePoint(0, 0)];
		}
		[self addCursorRect:self.bounds cursor:cursor];
	}
}

- (void) _surfaceNeedsUpdate:(NSNotification*)notification
{
   [self update];
}

- (void) lockFocus
{
	NSOpenGLContext* ctx = [self context];
	[super lockFocus];
	if ([ctx view] != self) {
		[ctx setView:self];
    }
    [ctx makeCurrentContext];
	if (!Application.isEditor)
	{
		/*int swapInterval = 1;
		[ctx setValues:&swapInterval forParameter:NSOpenGLCPSwapInterval]; */
	}
}

- (void) viewDidMoveToWindow
{
	[super viewDidMoveToWindow];
	if ([self window] == nil)
		[context clearDrawable];
}

- (void) drawRect:(NSRect)rect
{
	// better not to draw anything when the game has already finished
	if (Application.fQuitMsgReceived)
		return;
	// don't draw if tab-switched away from fullscreen
	if ([self.controller isFullScreenConsideringLionFullScreen] && ![NSApp isActive])
		return;
	if ([self.window isMiniaturized] || ![self.window isVisible])
		return;
	//[self.context update];
	C4Window* stdWindow = self.controller.stdWindow;
	
	if (stdWindow)
		stdWindow->PerformUpdate();
	else
		glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
}

- (ClonkWindowController*) controller {return (ClonkWindowController*)[self.window delegate];}

int32_t mouseButtonFromEvent(NSEvent* event, DWORD* modifierFlags)
{
	*modifierFlags = [event modifierFlags]; // should be compatible since MK_* constants mirror the NS* constants
	if ([event modifierFlags] & NSCommandKeyMask)
	{
		// treat cmd and ctrl the same
		*modifierFlags |= NSControlKeyMask;
	}
	switch (event.type)
	{
		case NSLeftMouseDown:
			return [event clickCount] > 1 ? C4MC_Button_LeftDouble : C4MC_Button_LeftDown;
		case NSLeftMouseUp:
			return C4MC_Button_LeftUp;
		case NSRightMouseDown:
			return [event clickCount] > 1 ? C4MC_Button_RightDouble : C4MC_Button_RightDown;
		case NSRightMouseUp:
			return C4MC_Button_RightUp;
		case NSLeftMouseDragged: case NSRightMouseDragged:
			return C4MC_Button_None; // sending mouse downs all the time when dragging is not the right thing to do
		case NSOtherMouseDown:
			return C4MC_Button_MiddleDown;
		case NSOtherMouseUp:
			return C4MC_Button_MiddleUp;
		case NSScrollWheel:
			return C4MC_Button_Wheel;
	}
	return C4MC_Button_None;
}

- (BOOL) shouldHideMouseCursor
{
	return !Application.isEditor || (self.controller.viewport && Console.EditCursor.GetMode() == C4CNS_ModePlay && ValidPlr(self.controller.viewport->GetPlayer()));
}

- (void) showCursor
{
	if ([self shouldHideMouseCursor])
		[NSCursor unhide];
}

- (void) hideCursor
{
	if ([self shouldHideMouseCursor])
		[NSCursor hide];
}

- (void) centerMouse
{
	savedMouse = CGPointMake(self.frame.size.width/2, self.frame.size.height/2);
	// emulate MouseEvent assuming this method is called in game mode
	::C4GUI::MouseMove(C4MC_Button_None,
		savedMouse.x*Config.Graphics.ResX/self.frame.size.width,
		savedMouse.y*Config.Graphics.ResY/self.frame.size.height,
		0, NULL
	);
}

- (void) mouseEvent:(NSEvent*)event
{
	DWORD flags = 0;
	int32_t button = mouseButtonFromEvent(event, &flags);
	int actualSizeX = Application.isEditor ? self.frame.size.width  : Config.Graphics.ResX;
	int actualSizeY = Application.isEditor ? self.frame.size.height : Config.Graphics.ResY;
	if (!Application.isEditor && lionAndBeyond() && (self.window.styleMask & NSFullScreenWindowMask) == NSFullScreenWindowMask)
	{
		//CGWarpMouseCursorPosition(CGPointMake(actualSizeX/2, actualSizeY/2));
		if (button != C4MC_Button_Wheel)
		{
			savedMouse.x += event.deltaX * (Config.Graphics.ResX/self.frame.size.width);
			savedMouse.y -= event.deltaY * (Config.Graphics.ResY/self.frame.size.height);
		}
	} else
	{
		savedMouse = [self convertPoint:[self.window mouseLocationOutsideOfEventStream] fromView:nil];
		if (!Application.isEditor)
		{
			savedMouse.x *= Config.Graphics.ResX/self.frame.size.width;
			savedMouse.y *= Config.Graphics.ResY/self.frame.size.height;
		}
	}
	savedMouse.x = fmin(fmax(savedMouse.x, 0), actualSizeX);
	savedMouse.y = fmin(fmax(savedMouse.y, 0), actualSizeY);
	int x = savedMouse.x;
	int y = actualSizeY - savedMouse.y;
	
	{
		C4Viewport* viewport = self.controller.viewport;
		if (::MouseControl.IsViewport(viewport) && Console.EditCursor.GetMode() == C4CNS_ModePlay)
		{	
			DWORD keyMask = flags;
			if ([event type] == NSScrollWheel)
				keyMask |= (int)[event deltaY] << 16;
			::C4GUI::MouseMove(button, x, y, keyMask, Application.isEditor ? viewport : NULL);
		}
		else if (viewport)
		{
			switch (button)
			{
			case C4MC_Button_LeftDown:
				Console.EditCursor.Move(viewport->ViewX+x/viewport->GetZoom(), viewport->ViewY+y/viewport->GetZoom(), flags);
				Console.EditCursor.LeftButtonDown(flags);
				break;
			case C4MC_Button_LeftUp:
				Console.EditCursor.LeftButtonUp(flags);
				break;
			case C4MC_Button_RightDown:
				Console.EditCursor.RightButtonDown(flags);
				break;
			case C4MC_Button_RightUp:
				Console.EditCursor.RightButtonUp(flags);
				break;
			case C4MC_Button_None:
				Console.EditCursor.Move(viewport->ViewX+x/viewport->GetZoom(),viewport->ViewY+y/viewport->GetZoom(), flags);
				break;
			}
		}
	}

}

- (void) magnifyWithEvent:(NSEvent *)event
{
	if (Game.IsRunning && (event.modifierFlags & NSAlternateKeyMask) == 0)
	{
		C4Viewport* viewport = self.controller.viewport;
		if (viewport)
		{
			viewport->SetZoom(viewport->GetZoom()+[event magnification], true);
		}
	}
	else
	{
		if (lionAndBeyond())
			[self.window toggleFullScreen:self];
		else
			[self.controller setFullscreen:[event magnification] > 0];
	}

}

- (void) swipeWithEvent:(NSEvent*)event
{
	// swiping left triggers going back in startup dialogs
	if (event.deltaX > 0)
		[ClonkAppDelegate.instance simulateKeyPressed:K_LEFT];
	else
		[ClonkAppDelegate.instance simulateKeyPressed:K_RIGHT];
}

- (void)insertText:(id)insertString
{
	if (::pGUI)
	{
		NSString* str = [insertString isKindOfClass:[NSAttributedString class]] ? [(NSAttributedString*)insertString string] : (NSString*)insertString;
		const char* cstr = [str cStringUsingEncoding:NSUTF8StringEncoding];
		::pGUI->CharIn(cstr);
	}
}

- (void)doCommandBySelector:(SEL)selector
{
	// ignore to not trigger the annoying beep sound
}

- (void)keyEvent:(NSEvent*)event withKeyEventType:(C4KeyEventType)type
{
	Game.DoKeyboardInput(
		[event keyCode]+CocoaKeycodeOffset, // offset keycode by some value to distinguish between those special key defines
		type,
		[event modifierFlags] & NSAlternateKeyMask,
		([event modifierFlags] & NSControlKeyMask) || ([event modifierFlags] & NSCommandKeyMask),
		[event modifierFlags] & NSShiftKeyMask,
		false, NULL
	);
}

- (void)keyDown:(NSEvent*)event
{
	[self interpretKeyEvents:[NSArray arrayWithObject:event]]; // call this to route character input to insertText:
	[self keyEvent:event withKeyEventType:KEYEV_Down];
}

- (void)keyUp:(NSEvent*)event
{
	[self keyEvent:event withKeyEventType:KEYEV_Up];
}

- (NSDragOperation) draggingEntered:(id<NSDraggingInfo>)sender
{
	return NSDragOperationCopy;
}

- (NSDragOperation) draggingUpdated:(id<NSDraggingInfo>)sender
{
	return NSDragOperationCopy;
}

- (BOOL) prepareForDragOperation:(id<NSDraggingInfo>)sender
{
	return YES;
}

- (BOOL) performDragOperation:(id<NSDraggingInfo>)sender
{
	NSPasteboard* pasteboard = [sender draggingPasteboard];
	if ([[pasteboard types] containsObject:NSFilenamesPboardType])
	{
		NSArray* files = [pasteboard propertyListForType:NSFilenamesPboardType];
		C4Viewport* viewport = self.controller.viewport;
		if (viewport)
		{
			for (NSString* fileName in files)
			{
				viewport->DropFile([fileName cStringUsingEncoding:NSUTF8StringEncoding], [sender draggingLocation].x, [sender draggingLocation].y);
			}
		}
	}
	return YES;
}

- (void )concludeDragOperation:(id<NSDraggingInfo>)sender
{
}

// respaghettisize :D

- (void) scrollWheel:(NSEvent *)event
{
	if (!Application.isEditor)
		[self mouseEvent:event];
	else
	{
		C4Viewport* viewport = self.controller.viewport;
		if (viewport)
		{
			NSScrollView* scrollView = self.controller.scrollView;
			NSPoint p = NSMakePoint(2*-[event deltaX]/abs(GBackWdt-viewport->ViewWdt), 2*-[event deltaY]/abs(GBackHgt-viewport->ViewHgt));
			[scrollView.horizontalScroller setDoubleValue:scrollView.horizontalScroller.doubleValue+p.x];
			[scrollView.verticalScroller setDoubleValue:scrollView.verticalScroller.doubleValue+p.y];
			viewport->ViewPositionByScrollBars();
			[self display];
		}
	}

}

- (void) mouseDown:        (NSEvent *)event {[self mouseEvent:event];}
- (void) rightMouseDown:   (NSEvent *)event {[self mouseEvent:event];}
- (void) rightMouseUp:     (NSEvent *)event {[self mouseEvent:event];}
- (void) otherMouseDown:   (NSEvent *)event {[self mouseEvent:event];}
- (void) otherMouseUp:     (NSEvent *)event {[self mouseEvent:event];}
- (void) mouseUp:          (NSEvent *)event {[self mouseEvent:event];}
- (void) mouseMoved:       (NSEvent *)event {[self mouseEvent:event];}
- (void) mouseDragged:     (NSEvent *)event {[self mouseEvent:event];}
- (void) rightMouseDragged:(NSEvent *)event {[self mouseEvent:event];}

- (void) update
{
	[context update];
}

+ (CGDirectDisplayID) displayID
{
	return (CGDirectDisplayID)[[[[[NSApp keyWindow] screen] deviceDescription] valueForKey:@"NSScreenNumber"] intValue];
}

+ (void) enumerateMultiSamples:(std::vector<int>&)samples
{
	CGDirectDisplayID displayID = self.displayID;
	CGOpenGLDisplayMask displayMask = CGDisplayIDToOpenGLDisplayMask(displayID);
	int numRenderers = 0;
	CGLRendererInfoObj obj = NULL;
	GLint sampleModes;
	
	CGLQueryRendererInfo(displayMask, &obj, &numRenderers);
	CGLDescribeRenderer(obj, 0, kCGLRPSampleModes, &sampleModes);
	CGLDestroyRendererInfo(obj);
	
	if (sampleModes & kCGLMultisampleBit)
	{
		samples.push_back(1);
		samples.push_back(2);
		samples.push_back(4);
	}
}

+ (void) setSurfaceBackingSizeOf:(NSOpenGLContext*) context width:(int)wdt height:(int)hgt
{
	if (context && !Application.isEditor)
	{
		// Make back buffer size fixed ( http://developer.apple.com/library/mac/#documentation/GraphicsImaging/Conceptual/OpenGL-MacProgGuide/opengl_contexts/opengl_contexts.html )
		GLint dim[2] = {wdt, hgt};
		CGLContextObj ctx = (CGLContextObj)context.CGLContextObj;
		CGLSetParameter(ctx, kCGLCPSurfaceBackingSize, dim);
		CGLEnable (ctx, kCGLCESurfaceBackingSize);	
	}
}

- (void) setContextSurfaceBackingSizeToOwnDimensions
{
	[ClonkOpenGLView setSurfaceBackingSizeOf:self.context width:self.frame.size.width height:self.frame.size.height];
}

static NSOpenGLContext* MainContext;

+ (NSOpenGLContext*) mainContext
{
	return MainContext;
}

+ (NSOpenGLContext*) createContext:(CStdGLCtx*) pMainCtx
{
	std::vector<NSOpenGLPixelFormatAttribute> attribs;
	attribs.push_back(NSOpenGLPFADepthSize);
	attribs.push_back(16);
	if (!Application.isEditor && Config.Graphics.MultiSampling > 0)
	{
		std::vector<int> samples;
		[self enumerateMultiSamples:samples];
		if (!samples.empty())
		{
			attribs.push_back(NSOpenGLPFAMultisample);
			attribs.push_back(NSOpenGLPFASampleBuffers);
			attribs.push_back(1);
			attribs.push_back(NSOpenGLPFASamples);
			attribs.push_back(Config.Graphics.MultiSampling);
		}
	}
	attribs.push_back(NSOpenGLPFANoRecovery);
	//attribs.push_back(NSOpenGLPFADoubleBuffer);
	attribs.push_back(NSOpenGLPFAWindow);
	attribs.push_back(0);
	NSOpenGLPixelFormat* format = [[[NSOpenGLPixelFormat alloc] initWithAttributes:&attribs[0]] autorelease];

	NSOpenGLContext* result = [[NSOpenGLContext alloc] initWithFormat:format shareContext:pMainCtx ? (NSOpenGLContext*)pMainCtx->GetNativeCtx() : nil];
	[self setSurfaceBackingSizeOf:result width:Config.Graphics.ResX height:Config.Graphics.ResY];
	if (!MainContext)
	{
		MainContext = result;
	}
	return result;
}

@end

@implementation ClonkEditorOpenGLView

- (void) copy:(id) sender
{
	Console.EditCursor.Duplicate();
}

- (void) delete:(id) sender
{
	Console.EditCursor.Delete();
}

- (IBAction) grabContents:(id) sender
{
	Console.EditCursor.GrabContents();
}

- (IBAction) resetZoom:(id) sender
{
	self.controller.viewport->SetZoom(1, true);
}

- (IBAction) increaseZoom:(id)sender
{
	C4Viewport* v = self.controller.viewport;
	v->SetZoom(v->GetZoom()*2, false);
}

- (IBAction) decreaseZoom:(id)sender
{
	C4Viewport* v = self.controller.viewport;
	v->SetZoom(v->GetZoom()/2, false);
}

@end

#pragma mark CStdGLCtx: Initialization

void* CStdGLCtx::GetNativeCtx()
{
	return ctx;
}

CStdGLCtx::CStdGLCtx(): pWindow(0), ctx(nil) {}

void CStdGLCtx::Clear()
{
	Deselect();
	if (ctx)
	{
		[(NSOpenGLContext*)ctx release];
		ctx = nil;
	}
	pWindow = 0;
}

void C4Window::EnumerateMultiSamples(std::vector<int>& samples) const
{
	[ClonkOpenGLView enumerateMultiSamples:samples];
}

bool C4AbstractApp::SetVideoMode(unsigned int iXRes, unsigned int iYRes, unsigned int iColorDepth, unsigned int iRefreshRate, unsigned int iMonitor, bool fFullScreen)
{
	fFullScreen &= !lionAndBeyond(); // Always false for Lion since then Lion's true(tm) Fullscreen is used
	ClonkWindowController* controller = (ClonkWindowController*)pWindow->GetController();
	NSWindow* window = controller.window;

	if (iXRes == -1 && iYRes == -1)
	{
		iXRes = CGDisplayPixelsWide(ClonkOpenGLView.displayID);
		iYRes = CGDisplayPixelsHigh(ClonkOpenGLView.displayID);
	}
	pWindow->SetSize(iXRes, iYRes);
	[controller setFullscreen:fFullScreen];
	[window setAspectRatio:[[window contentView] frame].size];
	[window center];
	if (!fFullScreen)
		[window makeKeyAndOrderFront:nil];
	OnResolutionChanged(iXRes, iYRes);
	return true;
}

bool CStdGLCtx::Init(C4Window * pWindow, C4AbstractApp *)
{
	// safety
	if (!pGL) return false;
	// store window
	this->pWindow = pWindow;
	// Create Context with sharing (if this is the main context, our ctx will be 0, so no sharing)
	// try direct rendering first
	NSOpenGLContext* ctx = [ClonkOpenGLView createContext:pGL->pMainCtx];
	this->ctx = (void*)ctx;
	// No luck at all?
	if (!Select(true)) return pGL->Error("  gl: Unable to select context");
	// init extensions
	GLenum err = glewInit();
	if (GLEW_OK != err)
	{
		// Problem: glewInit failed, something is seriously wrong.
		return pGL->Error(reinterpret_cast<const char*>(glewGetErrorString(err)));
	}
	// set the openglview's context
	ClonkWindowController* controller = (ClonkWindowController*)pWindow->GetController();
	if (controller && controller.openGLView)
	{
		[controller.openGLView setContext:ctx];
	}
	return true;
}

#pragma mark CStdGLCtx: Select/Deselect

bool CStdGLCtx::Select(bool verbose)
{
	if (ctx)
		[(NSOpenGLContext*)ctx makeCurrentContext];
	SelectCommon();
	// update clipper - might have been done by UpdateSize
	// however, the wrong size might have been assumed
	if (!pGL->UpdateClipper())
	{
		if (verbose) pGL->Error("  gl: UpdateClipper failed");
		return false;
	}
	// success
	return true;
}

void CStdGLCtx::Deselect()
{
	if (pGL && pGL->pCurrCtx == this)
    {
		pGL->pCurrCtx = 0;
    }
}

bool CStdGLCtx::PageFlip()
{
	// flush GL buffer
	glFlush();
	if (!pWindow) return false;
	//SDL_GL_SwapBuffers();
	return true;
}

#pragma mark CStdGLCtx: Gamma

bool C4AbstractApp::ApplyGammaRamp(struct _D3DGAMMARAMP &ramp, bool fForce)
{
	CGGammaValue r[256];
	CGGammaValue g[256];
	CGGammaValue b[256];
	for (int i = 0; i < 256; i++)
	{
		r[i] = static_cast<float>(ramp.red[i])/65535.0;
		g[i] = static_cast<float>(ramp.green[i])/65535.0;
		b[i] = static_cast<float>(ramp.blue[i])/65535.0;
	}
	CGSetDisplayTransferByTable(ClonkOpenGLView.displayID, 256, r, g, b);
	return true;
}

bool C4AbstractApp::SaveDefaultGammaRamp(_D3DGAMMARAMP &ramp)
{
	CGGammaValue r[256];
	CGGammaValue g[256];
	CGGammaValue b[256];
	uint32_t count;
	CGGetDisplayTransferByTable(ClonkOpenGLView.displayID, 256, r, g, b, &count);
	for (int i = 0; i < 256; i++)
	{
		ramp.red[i]   = r[i]*65535;
		ramp.green[i] = g[i]*65535;
		ramp.blue[i]  = b[i]*65535;
	}
	return true;

}

#endif
