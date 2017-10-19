
#import "ConsoleWindowController.h"

@interface ConsoleWindowController ()

@end
NSThread *msgThread;	//add by James
#define SKIP_LINES_COUNT    3
#define MAX_LINE_LEN        4096
#define MAX_LINES_COUNT     200
#define MAX_LEN        120	//add by James
@implementation ConsoleWindowController
@synthesize textView;

BOOL beScroll;	//add by James

- (id)initWithWindow:(NSWindow *)window
{
    self = [super initWithWindow:window];
    if (self)
    {
        // Initialization code here.
        linesCount = [[NSMutableArray arrayWithCapacity:MAX_LINES_COUNT + 1] retain];
        // add by James
        msgArray = [[NSMutableArray alloc] init];
        msgThread = [NSThread new] ;
        beScroll = true;
    }

    return self;
}

- (void)dealloc
{
    [linesCount release];
	// add by James
    [msgArray release];
    [msgThread cancel];
    [msgThread release];
    [super dealloc];
}

- (void)windowDidLoad
{
    [super windowDidLoad];
    // Implement this method to handle any initialization after your window controller's window has been loaded from its nib file.
}


- (void) trace:(NSString*)msg
{
// edit by James
    /*
    if (traceCount >= SKIP_LINES_COUNT && [msg length] > MAX_LINE_LEN)
    {
        msg = [NSString stringWithFormat:@"%@ ...", [msg substringToIndex:MAX_LINE_LEN - 4]];
    }
    traceCount++;
    NSFont *font = [NSFont fontWithName:@"Monaco" size:12.0];
    NSDictionary *attrsDictionary = [NSDictionary dictionaryWithObject:font forKey:NSFontAttributeName];
    NSAttributedString *string = [[NSAttributedString alloc] initWithString:msg attributes:attrsDictionary];
    NSNumber *len = [NSNumber numberWithUnsignedInteger:[string length]];
    [linesCount addObject:len];

	NSTextStorage *storage = [textView textStorage];
	[storage beginEditing];
	[storage appendAttributedString:string];

    if ([linesCount count] >= MAX_LINES_COUNT)
    {
        len = [linesCount objectAtIndex:0];
        [storage deleteCharactersInRange:NSMakeRange(0, [len unsignedIntegerValue])];
        [linesCount removeObjectAtIndex:0];
    }

	[storage endEditing];
    [self changeScroll];
    */
    
    //[self performSelectorInBackground:@selector(traceInNewThread:) withObject:msg];
    //[self performSelector:@selector(traceInNewThread:) onThread:msgThread withObject:msg waitUntilDone:YES];
    [self traceInNewThread:msg];
}

- (void) changeScroll
{
    BOOL scroll = [checkScroll state] == NSOnState;
    if(scroll)
    {
        [self.textView scrollRangeToVisible: NSMakeRange(self.textView.string.length, 0)];
    }
}

- (IBAction)onClear:(id)sender
{
	// edit by James
    [msgArray removeAllObjects];
    [tableView reloadData];
    /*
    NSTextStorage *storage = [textView textStorage];
    [storage setAttributedString:[[[NSAttributedString alloc] initWithString:@""] autorelease]];
     */
}

- (IBAction)onScrollChange:(id)sender
{
	// edit by James
    beScroll = !beScroll;
    if (beScroll == true)
        [tableView scrollRowToVisible:[msgArray count]-1];
    //[self changeScroll];
}

- (IBAction)onTopChange:(id)sender
{
    BOOL isTop = [topCheckBox state] == NSOnState;
    if(isTop)
    {
        [self.window setLevel:NSFloatingWindowLevel];
    }
    else
    {
        [self.window setLevel:NSNormalWindowLevel];
    }
}

// add by James
- (NSDictionary*) msgToDict:(NSString*)msg preColor:(NSColor*)perColor
{
    BOOL hasColor = false;
    NSMutableDictionary *dict = [[[NSMutableDictionary alloc] init] autorelease];
    if ([msg rangeOfString:@"[LUA-DEBUG"].length > 0 || [msg rangeOfString:@"[C++ DEBUG"].length > 0)
    {
        [dict setObject:[NSColor brownColor] forKey:@"color"];
        hasColor = true;
    }
    else if ([msg rangeOfString:@"[LUA-INFO"].length > 0 || [msg rangeOfString:@"[C++  INFO"].length > 0)
    {
        [dict setObject:[NSColor greenColor] forKey:@"color"];
        hasColor = true;
    }
    else if ([msg rangeOfString:@"[LUA-WARN"].length > 0 || [msg rangeOfString:@"[C++  WARN"].length > 0)
    {
        [dict setObject:[NSColor purpleColor] forKey:@"color"];
        hasColor = true;
    }
    else if ([msg rangeOfString:@"[LUA-ERROR"].length > 0 || [msg rangeOfString:@"[C++ ERROR"].length > 0 || [msg rangeOfString:@"[LUA-print] LUA ERROR"].length > 0)
    {
        [dict setObject:[NSColor redColor] forKey:@"color"];
        hasColor = true;
    }
    
    if (perColor && hasColor == false)
    {
        [dict setObject:perColor forKey:@"color"];
    }
    [dict setObject:msg forKey:@"msg"];
    return dict;
}

- (void) traceInNewThread:(NSString*)msg
{
    if ([msg length] > 1)
    {
        NSMutableArray *tempArray = [[NSMutableArray alloc] init];
        //[tempArray removeAllObjects];
        
        NSRange range = [msg rangeOfString:@"\n"];
        while (range.length > 0) {
            [ tempArray addObject: [msg substringToIndex:range.location] ];
            msg = [msg substringFromIndex:range.location+range.length];
            range = [msg rangeOfString:@"\n"];
        }
        [tempArray addObject:msg];
        while ([msgArray count]>1 && [msgArray count]+[tempArray count] > 300) {
            [msgArray removeObjectAtIndex:0];
        }
        NSColor *preColor = nil;
        for (NSString *subMsg in tempArray) {
            while ([subMsg length] > MAX_LEN) {
                NSString *resMsg = [subMsg substringToIndex:MAX_LEN];
                NSDictionary *dict = [self msgToDict:resMsg preColor:preColor];
                preColor = [dict objectForKey:@"color"];
                [msgArray addObject:dict];
                //[msgArray addObject:resMsg];
                subMsg = [subMsg substringFromIndex:MAX_LEN];
            }
            if ([subMsg length] > 0)
            {
                NSDictionary *dict = [self msgToDict:subMsg preColor:preColor];
                preColor = [dict objectForKey:@"color"];
                [msgArray addObject:dict];
            }
            //[msgArray addObject:[self msgToDict:subMsg preColor:preColor]];
            //[msgArray addObject:subMsg];
        }
        [tempArray release];
        [self performSelectorOnMainThread:@selector(traceDone) withObject:nil waitUntilDone:NO];
    }
}

- (void) traceDone
{
    [tableView reloadData];
    if (beScroll == true)
        [tableView scrollRowToVisible:[msgArray count]-1];
}

- (NSView *)tableView:(NSTableView *)tableView viewForTableColumn:(NSTableColumn *)tableColumn row:(NSInteger)row {
    
    // Get a new ViewCell
    NSTableCellView *cellView = [tableView makeViewWithIdentifier:tableColumn.identifier owner:self];
    
    // Since this is a single-column table view, this would not be necessary.
    // But it's a good practice to do it in order by remember it when a table is multicolumn.
    NSDictionary *dict = [msgArray objectAtIndex:row];
    NSString *msg = [dict objectForKey:@"msg"];
    if (msg) {
        cellView.textField.stringValue = msg;
    }
    NSColor *color = [dict objectForKey:@"color"];    
    if (color) {
        [cellView.textField setTextColor:color];
    }
    
    return cellView;
}


- (NSInteger)numberOfRowsInTableView:(NSTableView *)tableView {
    return [msgArray count];
}
@end
