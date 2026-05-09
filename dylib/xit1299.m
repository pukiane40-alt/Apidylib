/*
 * xit1299.m — XIT1299 License Protection
 * Objective-C implementation for macOS / iOS
 *
 * Dialogs:
 *   macOS → NSAlert + NSTextField (AppKit)
 *   iOS   → UIAlertController + UITextField (UIKit)
 *
 * HTTP → NSURLSession (synchronous via semaphore)
 * Device ID → IOKit UUID (macOS) / UIDevice (iOS)
 */

#import <Foundation/Foundation.h>

#if TARGET_OS_IPHONE
  #import <UIKit/UIKit.h>
#else
  #import <AppKit/AppKit.h>
  #import <IOKit/IOKitLib.h>
#endif

#include "xit1299.h"

/* ══════════════════════════════════════════════════════════════
   Device ID
   ══════════════════════════════════════════════════════════════ */

static NSString* get_device_id(void) {
#if TARGET_OS_IPHONE
    /* iOS — identifierForVendor (unique per app+device) */
    NSString* idfv = [[[UIDevice currentDevice] identifierForVendor] UUIDString];
    if (idfv) return [@"IOS:" stringByAppendingString:idfv];
    return @"IOS:unknown";
#else
    /* macOS — IOPlatformUUID (hardware UUID, survives reinstalls) */
    io_service_t service = IOServiceGetMatchingService(
        kIOMasterPortDefault,
        IOServiceMatching("IOPlatformExpertDevice"));

    if (service) {
        CFTypeRef uuidRef = IORegistryEntryCreateCFProperty(
            service,
            CFSTR("IOPlatformUUID"),
            kCFAllocatorDefault, 0);
        IOObjectRelease(service);

        if (uuidRef) {
            NSString* uuid = (__bridge_transfer NSString*)uuidRef;
            return [@"MAC:" stringByAppendingString:uuid];
        }
    }
    /* Fallback: hostname */
    return [@"MAC-HOST:" stringByAppendingString:
            [[NSHost currentHost] localizedName] ?: @"unknown"];
#endif
}

/* ══════════════════════════════════════════════════════════════
   HTTP POST (synchronous via semaphore)
   ══════════════════════════════════════════════════════════════ */

typedef struct {
    BOOL    success;
    int     status;
    NSString* body;
} HttpResult;

static HttpResult http_post(NSString* url_str, NSDictionary* body_dict) {
    HttpResult r = {NO, 0, nil};

    NSURL* url = [NSURL URLWithString:url_str];
    if (!url) return r;

    NSMutableURLRequest* req = [NSMutableURLRequest requestWithURL:url
        cachePolicy:NSURLRequestReloadIgnoringLocalCacheData
        timeoutInterval:10.0];
    req.HTTPMethod = @"POST";
    [req setValue:@"application/json" forHTTPHeaderField:@"Content-Type"];

    NSError* err = nil;
    req.HTTPBody = [NSJSONSerialization dataWithJSONObject:body_dict
        options:0 error:&err];
    if (err || !req.HTTPBody) return r;

    dispatch_semaphore_t sem = dispatch_semaphore_create(0);
    __block HttpResult res = {NO, 0, nil};

    NSURLSessionTask* task = [[NSURLSession sharedSession]
        dataTaskWithRequest:req
        completionHandler:^(NSData* data, NSURLResponse* resp, NSError* e) {
            if (!e && data) {
                res.status  = (int)[(NSHTTPURLResponse*)resp statusCode];
                res.body    = [[NSString alloc] initWithData:data
                               encoding:NSUTF8StringEncoding];
                res.success = YES;
            }
            dispatch_semaphore_signal(sem);
        }];
    [task resume];
    dispatch_semaphore_wait(sem, dispatch_time(DISPATCH_TIME_NOW,
        (int64_t)(12 * NSEC_PER_SEC)));

    return res;
}

/* ══════════════════════════════════════════════════════════════
   Validate API call
   ══════════════════════════════════════════════════════════════ */

typedef struct {
    BOOL      valid;
    BOOL      device_locked;
    NSString* message;
    NSString* time_remaining;
} ValidateResult;

static ValidateResult call_validate(NSString* api_base,
                                    NSString* key,
                                    NSString* device_id) {
    ValidateResult r = {NO, NO,
        @"Cannot reach @xit1299 server. Check your connection.", @""};

    /* Strip trailing slash */
    if ([api_base hasSuffix:@"/"]) {
        api_base = [api_base substringToIndex:api_base.length - 1];
    }
    NSString* url = [api_base stringByAppendingString:@"/keys/validate"];

    NSDictionary* body = @{@"key": key, @"deviceId": device_id};
    HttpResult hr = http_post(url, body);

    if (!hr.success || hr.status != 200) {
        if (hr.status > 0)
            r.message = [NSString stringWithFormat:
                @"Server error (HTTP %d)", hr.status];
        return r;
    }

    NSData* d = [hr.body dataUsingEncoding:NSUTF8StringEncoding];
    NSDictionary* json = [NSJSONSerialization JSONObjectWithData:d
        options:0 error:nil];
    if (!json) return r;

    r.valid          = [json[@"valid"] boolValue];
    r.device_locked  = [json[@"deviceLocked"] boolValue];
    r.message        = json[@"message"] ?: @"";
    r.time_remaining = json[@"timeRemaining"] ?: @"";
    return r;
}

/* ══════════════════════════════════════════════════════════════
   macOS Dialogs (AppKit)
   ══════════════════════════════════════════════════════════════ */

#if !TARGET_OS_IPHONE

/* Custom NSView for the activation dialog header */
@interface XIT1299HeaderView : NSView
@end
@implementation XIT1299HeaderView
- (void)drawRect:(NSRect)dirtyRect {
    /* Dark background */
    [[NSColor colorWithRed:0.08 green:0.08 blue:0.10 alpha:1.0] setFill];
    NSRectFill(self.bounds);
    /* Cyan accent bar top */
    [[NSColor colorWithRed:0.0 green:0.78 blue:0.86 alpha:1.0] setFill];
    NSRectFill(NSMakeRect(0, self.bounds.size.height - 4,
                          self.bounds.size.width, 4));
    /* @xit1299 title */
    NSDictionary* attrs = @{
        NSFontAttributeName: [NSFont boldSystemFontOfSize:18],
        NSForegroundColorAttributeName:
            [NSColor colorWithRed:0.0 green:0.78 blue:0.86 alpha:1.0]
    };
    [@"@xit1299" drawAtPoint:NSMakePoint(20, 16) withAttributes:attrs];
}
@end

static NSString* show_activate_dialog_mac(void) {
    NSAlert* alert = [[NSAlert alloc] init];
    alert.messageText    = @"@xit1299";
    alert.informativeText = @"Enter your @xit1299 key to continue.";
    [alert addButtonWithTitle:@"Activate"];
    [alert addButtonWithTitle:@"Quit"];

    /* Text field for key input */
    NSTextField* tf = [[NSTextField alloc]
        initWithFrame:NSMakeRect(0, 0, 340, 28)];
    tf.placeholderString = @"Paste license key  (XIT-XXXXXXXX-XXXXXXXX-XXXXXXXX)";
    tf.font = [NSFont monospacedSystemFontOfSize:12 weight:NSFontWeightRegular];
    tf.bezelStyle = NSTextFieldRoundedBezel;
    alert.accessoryView = tf;

    /* Appearance: dark */
    if (@available(macOS 10.14, *)) {
        alert.window.appearance =
            [NSAppearance appearanceNamed:NSAppearanceName(NSAppearanceNameDarkAqua)];
    }

    NSModalResponse resp = [alert runModal];
    if (resp == NSAlertFirstButtonReturn) {
        NSString* key = [tf.stringValue
            stringByTrimmingCharactersInSet:
            [NSCharacterSet whitespaceAndNewlineCharacterSet]];
        return key.length > 0 ? key : nil;
    }
    return nil; /* Quit */
}

static void show_success_dialog_mac(NSString* time_remaining) {
    NSAlert* alert = [[NSAlert alloc] init];
    alert.alertStyle   = NSAlertStyleInformational;
    alert.messageText  = @"@xit1299";
    alert.informativeText = [NSString stringWithFormat:
        @"✅  @xit1299 VIP activated\n"
        @"⏳  %@\n\n"
        @"🔒  Protected by @xit1299",
        time_remaining.length ? time_remaining : @"Key active"];
    [alert addButtonWithTitle:@"Enter App"];

    if (@available(macOS 10.14, *)) {
        alert.window.appearance =
            [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
    }
    [alert runModal];
}

static void show_error_dialog_mac(NSString* msg) {
    NSAlert* alert = [[NSAlert alloc] init];
    alert.alertStyle      = NSAlertStyleCritical;
    alert.messageText     = @"@xit1299 — License Error";
    alert.informativeText = msg;
    [alert addButtonWithTitle:@"OK"];
    if (@available(macOS 10.14, *)) {
        alert.window.appearance =
            [NSAppearance appearanceNamed:NSAppearanceNameDarkAqua];
    }
    [alert runModal];
}

#else
/* ══════════════════════════════════════════════════════════════
   iOS Dialogs (UIKit)
   ══════════════════════════════════════════════════════════════ */

static NSString* g_ios_key_result   = nil;
static BOOL      g_ios_did_activate = NO;

static UIViewController* top_vc(void) {
    UIViewController* vc =
        [UIApplication sharedApplication].keyWindow.rootViewController;
    while (vc.presentedViewController)
        vc = vc.presentedViewController;
    return vc;
}

static NSString* show_activate_dialog_ios(void) {
    g_ios_key_result   = nil;
    g_ios_did_activate = NO;

    dispatch_semaphore_t sem = dispatch_semaphore_create(0);

    dispatch_async(dispatch_get_main_queue(), ^{
        UIAlertController* ac = [UIAlertController
            alertControllerWithTitle:@"@xit1299"
            message:@"Enter your @xit1299 key to continue."
            preferredStyle:UIAlertControllerStyleAlert];

        [ac addTextFieldWithConfigurationHandler:^(UITextField* tf) {
            tf.placeholder   = @"Paste license key";
            tf.font          = [UIFont fontWithName:@"Courier" size:14];
            tf.autocorrectionType         = UITextAutocorrectionTypeNo;
            tf.autocapitalizationType     = UITextAutocapitalizationTypeAllCharacters;
            tf.clearButtonMode            = UITextFieldViewModeWhileEditing;
        }];

        UIAlertAction* activate = [UIAlertAction
            actionWithTitle:@"Activate"
            style:UIAlertActionStyleDefault
            handler:^(UIAlertAction* a) {
                NSString* k = ac.textFields.firstObject.text;
                k = [k stringByTrimmingCharactersInSet:
                    [NSCharacterSet whitespaceAndNewlineCharacterSet]];
                g_ios_key_result   = k.length ? k : nil;
                g_ios_did_activate = YES;
                dispatch_semaphore_signal(sem);
            }];

        UIAlertAction* quit = [UIAlertAction
            actionWithTitle:@"Quit"
            style:UIAlertActionStyleDestructive
            handler:^(UIAlertAction* a) {
                g_ios_did_activate = NO;
                dispatch_semaphore_signal(sem);
            }];

        [ac addAction:activate];
        [ac addAction:quit];
        [top_vc() presentViewController:ac animated:YES completion:nil];
    });

    dispatch_semaphore_wait(sem, dispatch_time(DISPATCH_TIME_NOW,
        (int64_t)(120 * NSEC_PER_SEC)));

    return g_ios_did_activate ? g_ios_key_result : nil;
}

static void show_success_dialog_ios(NSString* time_remaining) {
    dispatch_semaphore_t sem = dispatch_semaphore_create(0);
    dispatch_async(dispatch_get_main_queue(), ^{
        NSString* msg = [NSString stringWithFormat:
            @"✅  @xit1299 VIP activated\n⏳  %@\n\n🔒  Protected by @xit1299",
            time_remaining.length ? time_remaining : @"Key active"];

        UIAlertController* ac = [UIAlertController
            alertControllerWithTitle:@"@xit1299"
            message:msg
            preferredStyle:UIAlertControllerStyleAlert];

        [ac addAction:[UIAlertAction
            actionWithTitle:@"Enter App"
            style:UIAlertActionStyleDefault
            handler:^(UIAlertAction* a) {
                dispatch_semaphore_signal(sem);
            }]];

        [top_vc() presentViewController:ac animated:YES completion:nil];
    });
    dispatch_semaphore_wait(sem, dispatch_time(DISPATCH_TIME_NOW,
        (int64_t)(120 * NSEC_PER_SEC)));
}

static void show_error_dialog_ios(NSString* msg) {
    dispatch_semaphore_t sem = dispatch_semaphore_create(0);
    dispatch_async(dispatch_get_main_queue(), ^{
        UIAlertController* ac = [UIAlertController
            alertControllerWithTitle:@"@xit1299 — Error"
            message:msg
            preferredStyle:UIAlertControllerStyleAlert];
        [ac addAction:[UIAlertAction
            actionWithTitle:@"OK"
            style:UIAlertActionStyleDestructive
            handler:^(UIAlertAction* a) {
                dispatch_semaphore_signal(sem);
            }]];
        [top_vc() presentViewController:ac animated:YES completion:nil];
    });
    dispatch_semaphore_wait(sem, dispatch_time(DISPATCH_TIME_NOW,
        (int64_t)(30 * NSEC_PER_SEC)));
}

#endif /* iOS */

/* ══════════════════════════════════════════════════════════════
   Platform-agnostic wrappers
   ══════════════════════════════════════════════════════════════ */

static NSString* platform_show_activate(void) {
#if TARGET_OS_IPHONE
    return show_activate_dialog_ios();
#else
    return show_activate_dialog_mac();
#endif
}

static void platform_show_success(NSString* tr) {
#if TARGET_OS_IPHONE
    show_success_dialog_ios(tr);
#else
    show_success_dialog_mac(tr);
#endif
}

static void platform_show_error(NSString* msg) {
#if TARGET_OS_IPHONE
    show_error_dialog_ios(msg);
#else
    show_error_dialog_mac(msg);
#endif
}

/* ══════════════════════════════════════════════════════════════
   Public C API
   ══════════════════════════════════════════════════════════════ */

int XIT1299_ValidateSilent(const char* api_base_url,
                           const char* key,
                           const char* device_id) {
    if (!api_base_url || !key) return XIT1299_FAIL;

    NSString* base = @(api_base_url);
    NSString* k    = @(key);
    NSString* dev  = (device_id && device_id[0])
                     ? @(device_id) : get_device_id();

    ValidateResult r = call_validate(base, k, dev);
    return r.valid ? XIT1299_OK : XIT1299_FAIL;
}

int XIT1299_Activate(const char* api_base_url) {
    if (!api_base_url) return XIT1299_FAIL;

    NSString* base      = @(api_base_url);
    NSString* device_id = get_device_id();

    const int MAX_ATTEMPTS = 3;

    for (int attempt = 1; attempt <= MAX_ATTEMPTS; attempt++) {
        NSString* key = platform_show_activate();

        if (!key) return XIT1299_FAIL; /* user pressed Quit */

        ValidateResult vr = call_validate(base, key, device_id);

        if (vr.valid) {
            platform_show_success(vr.time_remaining);
            return XIT1299_OK;
        }

        NSString* err = vr.message.length
            ? vr.message : @"License key is not valid.";

        if (attempt < MAX_ATTEMPTS) {
            err = [err stringByAppendingFormat:
                @"\n\nAttempt %d of %d — try again.", attempt, MAX_ATTEMPTS];
        } else {
            err = [err stringByAppendingString:
                @"\n\nNo attempts remaining. Application will exit."];
        }
        platform_show_error(err);
    }

    return XIT1299_FAIL;
}
