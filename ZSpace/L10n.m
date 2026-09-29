#import "L10n.h"

@implementation L10n

+ (NSString *)localizedStringForKey:(NSString *)key {
    return NSLocalizedStringFromTable(key, @"develop", nil);
}

+ (NSString *)appTitle {
    return [self localizedStringForKey:@"zspace.dwg.app.title"];
}

+ (NSString *)openFile {
    return [self localizedStringForKey:@"zspace.dwg.action.open_file"];
}

+ (NSString *)emptyHint {
    return [self localizedStringForKey:@"zspace.dwg.empty.hint"];
}

@end
