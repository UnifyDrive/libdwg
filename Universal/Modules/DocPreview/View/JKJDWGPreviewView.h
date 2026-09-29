//
//  JKJDWGPreviewView.h
//  Local, read-only DWG renderer backed by LibreDWG.
//

#import <UIKit/UIKit.h>
#import <QuartzCore/CATiledLayer.h>
#import <CoreText/CoreText.h>
#import "../../../Components/dist-ios/include/dwg_api.h"

NS_ASSUME_NONNULL_BEGIN

@interface JKJDWGPreviewView : UIView
- (void)loadFile:(NSString *)filePath;
@end

NS_ASSUME_NONNULL_END

#if defined(JKJ_DWG_PREVIEW_IMPLEMENTATION)

@interface JKJDWGTextItem : NSObject
@property (nonatomic, copy) NSString *text;
@property (nonatomic, copy, nullable) NSString *fontName;
@property (nonatomic) CGPoint point;
@property (nonatomic) CGFloat height;
@property (nonatomic) CGFloat rotation;
@property (nonatomic) CGFloat widthFactor;
@property (nonatomic) CGFloat obliqueAngle;
@property (nonatomic) BOOL upsideDown;
@property (nonatomic) CGFloat alignmentLength;
@property (nonatomic) CGFloat horizontalAnchor;
@property (nonatomic) NSInteger verticalAnchor;
@property (nonatomic) BOOL verticalText;
@property (nonatomic) CGFloat boxWidth;
@property (nonatomic) CGFloat boxHeight;
@property (nonatomic) BOOL wrapsToBoxWidth;
@property (nonatomic) BOOL fitsToBoxHeight;
@property (nonatomic) BOOL clipsToBox;
@property (nonatomic) CGRect drawingBounds;
@property (nonatomic) uint32_t rgb;
@property (nonatomic) CGFloat alpha;
@property (nonatomic, copy) NSArray<UIBezierPath *> *clippingPaths;
@property (nonatomic) NSUInteger maskingPathStartIndex;
@end
@implementation JKJDWGTextItem
@end

@interface JKJDWGPathChunk : NSObject
@property (nonatomic) CGMutablePathRef path;
@property (nonatomic) CGRect drawingBounds;
@property (nonatomic) BOOL hasBounds;
@property (nonatomic) NSUInteger entityCount;
@property (nonatomic, copy) NSString *styleKey;
@property (nonatomic) uint32_t rgb;
@property (nonatomic) CGFloat alpha;
@property (nonatomic) CGFloat lineWidthPoints;
@property (nonatomic) CGFloat geometricLineWidth;
@property (nonatomic) BOOL fillPath;
@property (nonatomic) BOOL drawsPointMarkers;
@property (nonatomic, copy) NSArray<NSNumber *> *dashPattern;
@property (nonatomic, copy) NSArray<UIBezierPath *> *clippingPaths;
@end

@implementation JKJDWGPathChunk
- (instancetype)init {
    self = [super init];
    if (self) _path = CGPathCreateMutable();
    return self;
}
- (void)dealloc {
    if (_path) CGPathRelease(_path);
}
@end

@interface JKJDWGDrawing : NSObject
@property (nonatomic, copy) NSString *spaceName;
@property (nonatomic) BOOL activeSpace;
@property (nonatomic) CGRect drawingBounds;
@property (nonatomic) CGRect validationBounds;
@property (nonatomic) BOOL hasValidationBounds;
@property (nonatomic) NSUInteger skippedInvalidEntityCount;
@property (nonatomic) CGLineCap lineCap;
@property (nonatomic) CGLineJoin lineJoin;
@property (nonatomic, strong) NSMutableArray<JKJDWGPathChunk *> *pathChunks;
@property (nonatomic, strong) JKJDWGPathChunk *currentChunk;
@property (nonatomic, strong) NSMutableDictionary<NSString *, JKJDWGPathChunk *> *openChunksByStyle;
@property (nonatomic) NSUInteger styleWindowEntityCount;
@property (nonatomic, strong) NSMutableArray<JKJDWGTextItem *> *texts;
@property (nonatomic, strong) NSMutableArray<UIBezierPath *> *maskingPaths;
@property (nonatomic) NSUInteger entityCount;
@property (nonatomic, strong, nullable)
    NSDictionary<NSNumber *, NSIndexSet *> *blockEntityIndexesByOwnerHandle;
+ (nullable NSArray<JKJDWGDrawing *> *)drawingsWithFile:(NSString *)filePath error:(NSError **)error;
- (JKJDWGPathChunk *)chunkForNextEntityWithStyleKey:(NSString *)styleKey;
- (JKJDWGPathChunk *)chunkForNextEntity;
@end

static NSString * const JKJDWGErrorDomain = @"com.zspace.libredwg";
static const NSUInteger JKJDWGEntitiesPerPathChunk = 512;
static const CGFloat JKJDWGMinimumZoomScale = 1.0 / 1048576.0;
static const CGFloat JKJDWGMinimumTextHeight = 0.000001;
static const CGFloat JKJDWGMinimumRenderedTextSize = 0.05;

typedef struct {
    uint32_t rgb;
    // Effective color of the entity's layer. This deliberately differs from
    // rgb when an INSERT has an explicit color: BYBLOCK children inherit rgb,
    // while BYLAYER children on layer 0 inherit blockLayerRGB.
    uint32_t blockLayerRGB;
    CGFloat alpha;
    CGFloat lineWidthPoints;
    CGFloat geometricLineWidth;
    CGFloat dashLengths[32];
    NSUInteger dashCount;
    BOOL visible;
    BOOL fillPath;
    BOOL drawsPointMarkers;
    CGPathRef clipPaths[8];
    NSUInteger clipCount;
    NSUInteger clipEvenOddMask;
    uint64_t clipKey;
} JKJDWGResolvedStyle;

static JKJDWGResolvedStyle JKJDWGDefaultStyle(void) {
    JKJDWGResolvedStyle style = {0};
    style.rgb = 0xFFFFFF;
    style.blockLayerRGB = 0xFFFFFF;
    style.alpha = 1;
    style.lineWidthPoints = .7;
    style.visible = YES;
    return style;
}

static uint32_t JKJDWGRGBFromHSV(CGFloat hue, CGFloat saturation, CGFloat value) {
    CGFloat h = hue * 6;
    NSInteger sector = (NSInteger)floor(h) % 6;
    CGFloat fraction = h - floor(h);
    CGFloat p = value * (1 - saturation);
    CGFloat q = value * (1 - fraction * saturation);
    CGFloat t = value * (1 - (1 - fraction) * saturation);
    CGFloat r = 0, g = 0, b = 0;
    switch (sector) {
        case 0: r = value; g = t; b = p; break;
        case 1: r = q; g = value; b = p; break;
        case 2: r = p; g = value; b = t; break;
        case 3: r = p; g = q; b = value; break;
        case 4: r = t; g = p; b = value; break;
        default: r = value; g = p; b = q; break;
    }
    return ((uint32_t)lrint(r * 255) << 16) |
           ((uint32_t)lrint(g * 255) << 8) |
           (uint32_t)lrint(b * 255);
}

static uint32_t JKJDWGRGBFromACI(NSInteger index) {
    static const uint32_t basicColors[] = {
        0xFFFFFF, 0xFF0000, 0xFFFF00, 0x00FF00, 0x00FFFF,
        0x0000FF, 0xFF00FF, 0xFFFFFF, 0x808080, 0xC0C0C0
    };
    index = labs(index);
    if (index >= 0 && index <= 9) return basicColors[index];
    if (index >= 10 && index <= 249) {
        NSInteger hueIndex = (index - 10) / 10;
        NSInteger shade = (index - 10) % 10;
        static const CGFloat values[] = {1, 1, .65, .65, .5, .5, .3, .3, .15, .15};
        CGFloat saturation = shade % 2 == 0 ? 1 : .5;
        return JKJDWGRGBFromHSV((CGFloat)hueIndex / 24.0, saturation, values[shade]);
    }
    static const uint8_t gray[] = {51, 80, 105, 130, 190, 255};
    uint8_t value = gray[MIN(MAX(index - 250, 0), 5)];
    return ((uint32_t)value << 16) | ((uint32_t)value << 8) | value;
}

static void JKJDWGExpandBounds(CGRect *bounds, BOOL *hasBounds, double x, double y) {
    if (!isfinite(x) || !isfinite(y)) return;
    if (!*hasBounds) {
        *bounds = CGRectMake(x, y, 0, 0);
        *hasBounds = YES;
        return;
    }
    CGFloat minX = MIN(CGRectGetMinX(*bounds), x);
    CGFloat minY = MIN(CGRectGetMinY(*bounds), y);
    CGFloat maxX = MAX(CGRectGetMaxX(*bounds), x);
    CGFloat maxY = MAX(CGRectGetMaxY(*bounds), y);
    *bounds = CGRectMake(minX, minY, maxX - minX, maxY - minY);
}

static BOOL JKJDWGModelValidationBounds(Dwg_Data *dwg, CGRect *bounds) {
    if (!dwg || !bounds) return NO;
    CGFloat minX = dwg->header_vars.EXTMIN.x;
    CGFloat minY = dwg->header_vars.EXTMIN.y;
    CGFloat maxX = dwg->header_vars.EXTMAX.x;
    CGFloat maxY = dwg->header_vars.EXTMAX.y;
    if (!isfinite(minX) || !isfinite(minY) ||
        !isfinite(maxX) || !isfinite(maxY) ||
        maxX - minX <= DBL_EPSILON ||
        maxY - minY <= DBL_EPSILON) {
        return NO;
    }
    CGFloat width = maxX - minX;
    CGFloat height = maxY - minY;
    if (!isfinite(width) || !isfinite(height) ||
        width > 1e15 || height > 1e15) {
        return NO;
    }
    *bounds = CGRectMake(minX, minY, width, height);
    return YES;
}

static BOOL JKJDWGRectIsSane(JKJDWGDrawing *drawing, CGRect bounds) {
    if (CGRectIsNull(bounds) || CGRectIsInfinite(bounds) ||
        !isfinite(CGRectGetMinX(bounds)) ||
        !isfinite(CGRectGetMinY(bounds)) ||
        !isfinite(CGRectGetMaxX(bounds)) ||
        !isfinite(CGRectGetMaxY(bounds))) {
        return NO;
    }
    if (!drawing.hasValidationBounds) return YES;
    CGRect expected = drawing.validationBounds;
    CGFloat expectedWidth = MAX(CGRectGetWidth(expected), 1);
    CGFloat expectedHeight = MAX(CGRectGetHeight(expected), 1);
    if (CGRectGetWidth(bounds) > expectedWidth * 64 ||
        CGRectGetHeight(bounds) > expectedHeight * 64) {
        return NO;
    }
    CGRect generousBounds =
        CGRectInset(expected, -expectedWidth * 2, -expectedHeight * 2);
    return CGRectGetMaxX(bounds) >= CGRectGetMinX(generousBounds) &&
           CGRectGetMinX(bounds) <= CGRectGetMaxX(generousBounds) &&
           CGRectGetMaxY(bounds) >= CGRectGetMinY(generousBounds) &&
           CGRectGetMinY(bounds) <= CGRectGetMaxY(generousBounds);
}

static NSString *JKJDWGReadText(void *entity, const char *name, const char *field) {
    char *value = NULL;
    int mustFree = 0;
    if (!dwg_dynapi_entity_utf8text(entity, name, field, &value, &mustFree, NULL) || !value) {
        return nil;
    }
    NSUInteger byteLength = strlen(value);
    NSString *text = [[NSString alloc] initWithBytes:value
                                               length:byteLength
                                             encoding:NSUTF8StringEncoding];
    // A few old drawings contain text bytes which LibreDWG exposes without a
    // completely valid UTF-8 conversion. Do not discard the entire entity just
    // because one byte sequence is invalid.
    if (!text && byteLength) {
        NSStringEncoding gb18030 =
            CFStringConvertEncodingToNSStringEncoding(kCFStringEncodingGB_18030_2000);
        text = [[NSString alloc] initWithBytes:value
                                        length:byteLength
                                      encoding:gb18030];
    }
    if (!text && byteLength) {
        text = [[NSString alloc] initWithBytes:value
                                        length:byteLength
                                      encoding:NSWindowsCP1252StringEncoding];
    }
    if (mustFree) free(value);
    return text;
}

static UIFont *JKJDWGFont(NSString *fontName, CGFloat size) {
    UIFont *font = fontName.length ? [UIFont fontWithName:fontName size:size] : nil;
    if (!font) font = [UIFont fontWithName:@"NotoSansSC-Regular" size:size];
    if (!font) font = [UIFont systemFontOfSize:size];
    return font;
}

static CGPathRef JKJDWGCreateTextGlyphPath(NSString *text,
                                           UIFont *font,
                                           CGFloat *lineWidth) {
    if (!text.length || !font) return NULL;
    CTFontRef baseFont = CTFontCreateWithName(
        (__bridge CFStringRef)font.fontName, font.pointSize, NULL);
    if (!baseFont) return NULL;
    NSAttributedString *attributedText =
        [[NSAttributedString alloc] initWithString:text
                                        attributes:@{
                                            (__bridge NSString *)kCTFontAttributeName:
                                                (__bridge id)baseFont
                                        }];
    CTLineRef line = CTLineCreateWithAttributedString(
        (__bridge CFAttributedStringRef)attributedText);
    if (!line) {
        CFRelease(baseFont);
        return NULL;
    }
    if (lineWidth) {
        *lineWidth = (CGFloat)CTLineGetTypographicBounds(
            line, NULL, NULL, NULL);
    }

    CGMutablePathRef textPath = CGPathCreateMutable();
    CFArrayRef runs = CTLineGetGlyphRuns(line);
    CFIndex runCount = CFArrayGetCount(runs);
    for (CFIndex runIndex = 0; runIndex < runCount; runIndex++) {
        CTRunRef run = (CTRunRef)CFArrayGetValueAtIndex(runs, runIndex);
        CFDictionaryRef runAttributes = CTRunGetAttributes(run);
        CTFontRef runFont = runAttributes
            ? (CTFontRef)CFDictionaryGetValue(runAttributes,
                                              kCTFontAttributeName)
            : NULL;
        if (!runFont) runFont = baseFont;
        CFIndex glyphCount = CTRunGetGlyphCount(run);
        for (CFIndex glyphIndex = 0; glyphIndex < glyphCount; glyphIndex++) {
            CGGlyph glyph = 0;
            CGPoint position = CGPointZero;
            CTRunGetGlyphs(run, CFRangeMake(glyphIndex, 1), &glyph);
            CTRunGetPositions(run, CFRangeMake(glyphIndex, 1), &position);
            CGPathRef glyphPath =
                CTFontCreatePathForGlyph(runFont, glyph, NULL);
            if (!glyphPath) continue;
            CGAffineTransform placement =
                CGAffineTransformMakeTranslation(position.x, position.y);
            CGPathAddPath(textPath, &placement, glyphPath);
            CGPathRelease(glyphPath);
        }
    }
    CFRelease(line);
    CFRelease(baseFont);
    if (CGPathIsEmpty(textPath)) {
        CGPathRelease(textPath);
        return NULL;
    }
    return textPath;
}

static NSString *JKJDWGVisibleFallbackText(NSString *source) {
    if (!source.length) return source;
    NSMutableString *visibleText = [NSMutableString string];
    NSMutableOrderedSet<NSString *> *missingCodes = [NSMutableOrderedSet orderedSet];
    [source enumerateSubstringsInRange:NSMakeRange(0, source.length)
                              options:NSStringEnumerationByComposedCharacterSequences
                           usingBlock:^(NSString *substring, NSRange substringRange,
                                        NSRange enclosingRange, BOOL *stop) {
        if (!substring.length) return;
        unichar first = [substring characterAtIndex:0];
        uint32_t scalar = first;
        if (scalar == '\r') {
            [visibleText appendString:@"\n"];
            return;
        }
        if (CFStringIsSurrogateHighCharacter(first) && substring.length > 1) {
            unichar second = [substring characterAtIndex:1];
            if (CFStringIsSurrogateLowCharacter(second)) {
                scalar = CFStringGetLongCharacterForSurrogatePair(first, second);
            }
        }
        BOOL invalidControl =
            scalar < 0x20 && scalar != '\n' && scalar != '\t';
        BOOL privateUse =
            (scalar >= 0xE000 && scalar <= 0xF8FF) ||
            (scalar >= 0xF0000 && scalar <= 0xFFFFD) ||
            (scalar >= 0x100000 && scalar <= 0x10FFFD);
        BOOL invalidScalar =
            scalar == 0xFFFD ||
            (CFStringIsSurrogateHighCharacter(first) && substring.length == 1) ||
            CFStringIsSurrogateLowCharacter(first);
        if (invalidControl || privateUse || invalidScalar) {
            [visibleText appendString:@"□"];
            [missingCodes addObject:
                [NSString stringWithFormat:@"U+%04X", (unsigned int)scalar]];
        } else {
            [visibleText appendString:substring];
        }
    }];
    if (missingCodes.count) {
        NSLog(@"[DWG] 缺少 SHX/Unicode 字形 %@，已使用占位符显示",
              [missingCodes.array componentsJoinedByString:@", "]);
    }
    return visibleText;
}

static NSString *JKJDWGDisplayText(NSString *source, BOOL isMText) {
    if (!source.length) return source;
    NSMutableString *result = [NSMutableString string];
    NSUInteger index = 0;
    while (index < source.length) {
        unichar character = [source characterAtIndex:index];

        if (character == '%' && index + 2 < source.length &&
            [source characterAtIndex:index + 1] == '%' &&
            [[NSCharacterSet decimalDigitCharacterSet]
                characterIsMember:[source characterAtIndex:index + 2]]) {
            NSUInteger digitStart = index + 2;
            NSUInteger digitEnd = digitStart;
            while (digitEnd < source.length && digitEnd - digitStart < 3 &&
                   [[NSCharacterSet decimalDigitCharacterSet]
                       characterIsMember:[source characterAtIndex:digitEnd]]) {
                digitEnd++;
            }
            NSString *digits =
                [source substringWithRange:NSMakeRange(digitStart,
                                                       digitEnd - digitStart)];
            NSInteger shapeCode = digits.integerValue;
            NSString *replacement = nil;
            switch (shapeCode) {
                case 176:
                case 248:
                    replacement = @"°";
                    break;
                case 177:
                case 241:
                    replacement = @"±";
                    break;
                case 216:
                    replacement = @"⌀";
                    break;
                default:
                    replacement = @"□";
                    NSLog(@"[DWG] 缺少 SHX 字形编号 %ld（原编码 %%%% %@），"
                          @"已使用占位符显示",
                          (long)shapeCode, digits);
                    break;
            }
            [result appendString:replacement];
            index = digitEnd;
            continue;
        }

        if (character == '%' && index + 2 < source.length &&
            [source characterAtIndex:index + 1] == '%') {
            unichar code = [source characterAtIndex:index + 2];
            if (code >= 'A' && code <= 'Z') code += 'a' - 'A';
            if (code == 'd') [result appendString:@"°"];
            else if (code == 'p') [result appendString:@"±"];
            else if (code == 'c') [result appendString:@"⌀"];
            else if (code == 'o' || code == 'u') {
                // SHX overline/underline toggles affect decoration only.
            }
            else {
                [result appendString:[source substringWithRange:NSMakeRange(index, 3)]];
            }
            index += 3;
            continue;
        }

        if (!isMText || character != '\\' || index + 1 >= source.length) {
            if (!isMText || (character != '{' && character != '}')) {
                [result appendFormat:@"%C", character];
            }
            index++;
            continue;
        }

        unichar command = [source characterAtIndex:index + 1];
        if (command == 'P') {
            [result appendString:@"\n"];
            index += 2;
            continue;
        }
        if (command == '~') {
            [result appendString:@" "];
            index += 2;
            continue;
        }
        if (command == '\\' || command == '{' || command == '}') {
            [result appendFormat:@"%C", command];
            index += 2;
            continue;
        }
        if ((command == 'L' || command == 'l' ||
             command == 'O' || command == 'o' ||
             command == 'K' || command == 'k')) {
            index += 2;
            continue;
        }
        if (command == 'U' && index + 6 < source.length &&
            [source characterAtIndex:index + 2] == '+') {
            NSString *hex = [source substringWithRange:NSMakeRange(index + 3, 4)];
            unsigned value = 0;
            NSScanner *scanner = [NSScanner scannerWithString:hex];
            if ([scanner scanHexInt:&value]) {
                [result appendFormat:@"%C", (unichar)value];
                index += 7;
                continue;
            }
        }

        NSRange terminator = [source rangeOfString:@";"
                                           options:0
                                             range:NSMakeRange(index + 2,
                                                               source.length - index - 2)];
        if (command == 'S' && terminator.location != NSNotFound) {
            NSString *stacked = [source substringWithRange:
                NSMakeRange(index + 2, terminator.location - index - 2)];
            stacked = [stacked stringByReplacingOccurrencesOfString:@"^" withString:@"/"];
            stacked = [stacked stringByReplacingOccurrencesOfString:@"#" withString:@"/"];
            [result appendString:stacked];
            index = NSMaxRange(terminator);
            continue;
        }
        BOOL isParameterCommand =
            command == 'f' || command == 'F' ||
            command == 'c' || command == 'C' ||
            command == 'h' || command == 'H' ||
            command == 'w' || command == 'W' ||
            command == 't' || command == 'T' ||
            command == 'q' || command == 'Q' ||
            command == 'a' || command == 'A' ||
            command == 'p';
        if (terminator.location != NSNotFound && isParameterCommand) {
            index = NSMaxRange(terminator);
            continue;
        }

        [result appendFormat:@"%C", command];
        index += 2;
    }
    return JKJDWGVisibleFallbackText(result);
}

static NSString *JKJDWGFontNameFromMText(NSString *source) {
    if (!source.length) return nil;
    NSRange fontCommand = [source rangeOfString:@"\\f" options:NSCaseInsensitiveSearch];
    if (fontCommand.location == NSNotFound) return nil;
    NSUInteger valueStart = NSMaxRange(fontCommand);
    NSRange terminator = [source rangeOfString:@";"
                                       options:0
                                         range:NSMakeRange(valueStart, source.length - valueStart)];
    if (terminator.location == NSNotFound) return nil;
    NSString *fontSpec = [source substringWithRange:
        NSMakeRange(valueStart, terminator.location - valueStart)];
    NSString *fontFamily = [[fontSpec componentsSeparatedByString:@"|"] firstObject];
    if ([fontFamily containsString:@"宋体"] ||
        [fontFamily caseInsensitiveCompare:@"SimSun"] == NSOrderedSame) {
        return @"STSongti-SC-Regular";
    }
    if ([fontFamily containsString:@"楷体"] ||
        [fontFamily caseInsensitiveCompare:@"KaiTi"] == NSOrderedSame) {
        return @"STKaitiSC-Regular";
    }
    if ([fontFamily containsString:@"仿宋"] ||
        [fontFamily caseInsensitiveCompare:@"FangSong"] == NSOrderedSame) {
        return @"STFangsong";
    }
    if ([fontFamily containsString:@"黑体"] ||
        [fontFamily caseInsensitiveCompare:@"SimHei"] == NSOrderedSame) {
        return @"PingFangSC-Regular";
    }
    return fontFamily.length ? fontFamily : nil;
}

static Dwg_Object_STYLE *JKJDWGTextStyle(Dwg_Data *dwg,
                                         BITCODE_H styleReference) {
    if (!dwg || !styleReference) return NULL;
    Dwg_Object *styleObject = dwg_ref_object(dwg, styleReference);
    if (!styleObject || styleObject->fixedtype != DWG_TYPE_STYLE ||
        !styleObject->tio.object) {
        return NULL;
    }
    return styleObject->tio.object->tio.STYLE;
}

static NSString *JKJDWGFontNameForTextStyle(Dwg_Data *dwg,
                                             BITCODE_H styleReference) {
    Dwg_Object_STYLE *style = JKJDWGTextStyle(dwg, styleReference);
    if (!style) return nil;
    NSString *fontFile = JKJDWGReadText(style, "STYLE", "font_file");
    NSString *bigFontFile = JKJDWGReadText(style, "STYLE", "bigfont_file");
    NSString *fontName =
        fontFile.lastPathComponent.lowercaseString.stringByDeletingPathExtension;
    NSString *bigFontName =
        bigFontFile.lastPathComponent.lowercaseString.stringByDeletingPathExtension;
    NSString *combined =
        [NSString stringWithFormat:@"%@ %@", fontName ?: @"", bigFontName ?: @""];
    if ([combined containsString:@"simhei"] ||
        [combined containsString:@"黑体"]) {
        return @"PingFangSC-Semibold";
    }
    if ([combined containsString:@"hzfs"] ||
        [combined containsString:@"fangsong"] ||
        [combined containsString:@"仿宋"]) {
        return @"STFangsong";
    }
    if ([combined containsString:@"gbcbig"] ||
        [combined containsString:@"hztxt"] ||
        [combined containsString:@"simsun"] ||
        [combined containsString:@"宋体"]) {
        return @"STSongti-SC-Regular";
    }
    if ([combined containsString:@"simplex"] ||
        [combined containsString:@"complex"] ||
        [combined containsString:@"gbenor"] ||
        [combined containsString:@"xiao"] ||
        [combined containsString:@"rs"]) {
        return @"Menlo-Regular";
    }
    if ([fontName containsString:@"arial"] ||
        [fontName containsString:@"helvetica"]) {
        return @"Helvetica";
    }
    return nil;
}

static BOOL JKJDWGTextStyleIsVertical(Dwg_Data *dwg, BITCODE_H styleReference) {
    Dwg_Object_STYLE *style = JKJDWGTextStyle(dwg, styleReference);
    return style && style->is_vertical;
}

static uint32_t JKJDWGRGBFromColor(Dwg_Color color, uint32_t fallbackRGB) {
    if (color.index == 0 || color.method == DWG_COLOR_METHOD_BYBLOCK) return fallbackRGB;
    if (color.index == 256 || color.method == DWG_COLOR_METHOD_BYLAYER) {
        return fallbackRGB;
    }
    if (color.method == DWG_COLOR_METHOD_TRUECOLOR) {
        uint32_t rgb = color.rgb & 0xFFFFFF;
        if (rgb == 0 || rgb == 0x100 || rgb == 0x101) return fallbackRGB;
        return rgb;
    }
    if (color.index > 0 && color.index < 256) return JKJDWGRGBFromACI(color.index);
    if (color.method == DWG_COLOR_METHOD_VOID) return fallbackRGB;
    return fallbackRGB;
}

static uint32_t JKJDWGRGBFromLayerColor(Dwg_Color color,
                                        uint32_t fallbackRGB) {
    // Some R2007 layer-table colors are exposed by LibreDWG as BYLAYER index
    // 256 plus a TRUECOLOR payload containing the original ACI value. A layer
    // cannot itself inherit BYLAYER, so recover that indexed color here. Keep
    // entity TRUECOLOR handling unchanged to avoid reinterpreting legitimate
    // dark RGB values.
    uint32_t payload = color.rgb & 0xFFFFFF;
    if (color.index == 256 &&
        color.method == DWG_COLOR_METHOD_TRUECOLOR &&
        payload > 0 && payload < 256) {
        return JKJDWGRGBFromACI(payload);
    }
    return JKJDWGRGBFromColor(color, fallbackRGB);
}

static CGFloat JKJDWGLineWidthPoints(BITCODE_RC entityLineWeight,
                                     BITCODE_RC layerLineWeight) {
    NSInteger lineWeight = entityLineWeight;
    if (lineWeight == 255 || lineWeight == 253) lineWeight = layerLineWeight;
    if (lineWeight == 254 || lineWeight > 211) lineWeight = 25;
    return MAX(.55, ((CGFloat)lineWeight / 100.0) * 2.83464567);
}

static NSUInteger JKJDWGDashPattern(Dwg_Data *dwg,
                                    Dwg_Object_Entity *entity,
                                    Dwg_Object_LAYER *layer,
                                    CGFloat lengths[32]) {
    BITCODE_H ltypeReference = entity->ltype;
    if (!ltypeReference || entity->ltype_flags == 0) ltypeReference = layer ? layer->ltype : NULL;
    if (!ltypeReference) return 0;
    Dwg_Object *ltypeObject = dwg_ref_object(dwg, ltypeReference);
    if (!ltypeObject || ltypeObject->fixedtype != DWG_TYPE_LTYPE ||
        !ltypeObject->tio.object || !ltypeObject->tio.object->tio.LTYPE) {
        return 0;
    }
    Dwg_Object_LTYPE *ltype = ltypeObject->tio.object->tio.LTYPE;
    if (!ltype->dashes || ltype->numdashes == 0) return 0;
    CGFloat entityScale = isfinite(entity->ltype_scale) && entity->ltype_scale > 0
        ? entity->ltype_scale : 1;
    CGFloat globalScale = isfinite(dwg->header_vars.LTSCALE) && dwg->header_vars.LTSCALE > 0
        ? dwg->header_vars.LTSCALE : 1;
    CGFloat scale = entityScale * globalScale;
    NSUInteger dashCount = MIN((NSUInteger)ltype->numdashes, 32);
    for (NSUInteger index = 0; index < dashCount; index++) {
        CGFloat length = fabs(ltype->dashes[index].length) * scale;
        lengths[index] = MAX(length, 0.01 * scale);
    }
    return dashCount;
}

static JKJDWGResolvedStyle JKJDWGStyleForObject(Dwg_Data *dwg,
                                                 Dwg_Object *object,
                                                 JKJDWGResolvedStyle inheritedStyle,
                                                 BOOL insideBlock) {
    JKJDWGResolvedStyle style = inheritedStyle;
    style.alpha = 1;
    style.geometricLineWidth = 0;
    style.visible = YES;
    style.fillPath = NO;
    if (!object || !object->tio.entity) {
        style.visible = NO;
        return style;
    }

    Dwg_Object_Entity *entity = object->tio.entity;
    Dwg_Object_LAYER *layer = dwg_get_entity_layer(entity);
    if (entity->invisible || (layer && (layer->off || layer->frozen || layer->color.index < 0))) {
        style.visible = NO;
        return style;
    }

    BOOL inheritsLayerZero = NO;
    uint32_t layerRGB = layer
        ? JKJDWGRGBFromLayerColor(layer->color, 0xFFFFFF) : 0xFFFFFF;
    if (insideBlock && layer) {
        NSString *layerName = JKJDWGReadText(layer, "LAYER", "name");
        inheritsLayerZero = [layerName isEqualToString:@"0"];
        if (inheritsLayerZero) layerRGB = inheritedStyle.blockLayerRGB;
    }
    style.blockLayerRGB = layerRGB;
    style.rgb = JKJDWGRGBFromColor(entity->color,
                                   entity->color.index == 0 ||
                                   entity->color.method == DWG_COLOR_METHOD_BYBLOCK
                                       ? inheritedStyle.rgb : layerRGB);
    if (entity->color.alpha_type == 3) {
        style.alpha = MAX(0.05, MIN((CGFloat)entity->color.alpha / 255.0, 1));
    } else if (entity->color.alpha_type == 1) {
        style.alpha = inheritedStyle.alpha;
    } else if (inheritsLayerZero) {
        style.alpha = inheritedStyle.alpha;
    } else if (layer && layer->color.alpha_type == 3) {
        style.alpha = MAX(0.05, MIN((CGFloat)layer->color.alpha / 255.0, 1));
    }
    if (!dwg->header_vars.LWDISPLAY) {
        style.lineWidthPoints = .7;
    } else if (entity->linewt == 254 ||
               (inheritsLayerZero &&
                (entity->linewt == 255 || entity->linewt == 253))) {
        style.lineWidthPoints = inheritedStyle.lineWidthPoints;
    } else {
        style.lineWidthPoints =
            JKJDWGLineWidthPoints(entity->linewt, layer ? layer->linewt : 25);
    }
    if (entity->ltype_flags == 1 ||
        (inheritsLayerZero && entity->ltype_flags == 0)) {
        style.dashCount = inheritedStyle.dashCount;
        for (NSUInteger index = 0; index < style.dashCount; index++) {
            style.dashLengths[index] = inheritedStyle.dashLengths[index];
        }
    } else {
        style.dashCount = JKJDWGDashPattern(dwg, entity, layer, style.dashLengths);
    }
    style.fillPath = object->fixedtype == DWG_TYPE_SOLID ||
                     object->fixedtype == DWG_TYPE_TRACE;
    if (object->fixedtype == DWG_TYPE_HATCH && entity->tio.HATCH) {
        Dwg_Entity_HATCH *hatch = entity->tio.HATCH;
        style.fillPath = hatch->is_solid_fill || hatch->is_gradient_fill;
        if (hatch->is_gradient_fill && hatch->num_colors > 0 && hatch->colors) {
            style.rgb = JKJDWGRGBFromColor(hatch->colors[0].color, style.rgb);
        }
    }
    return style;
}

static NSString *JKJDWGStyleKey(JKJDWGResolvedStyle style) {
    NSUInteger dashHash = 0;
    for (NSUInteger index = 0; index < style.dashCount; index++) {
        dashHash = dashHash * 31 + (NSUInteger)llround(style.dashLengths[index] * 1000);
    }
    return [NSString stringWithFormat:@"%06x-%.3f-%.3f-%.6f-%d-%d-%lu-%lu-%lu-%llu",
            style.rgb, style.alpha, style.lineWidthPoints, style.geometricLineWidth,
            style.fillPath, style.drawsPointMarkers,
            (unsigned long)style.dashCount, (unsigned long)dashHash,
            (unsigned long)style.clipEvenOddMask,
            (unsigned long long)style.clipKey];
}

static void JKJDWGExpandChunkBounds(JKJDWGPathChunk *chunk, double x, double y) {
    CGRect bounds = chunk.drawingBounds;
    BOOL hasBounds = chunk.hasBounds;
    JKJDWGExpandBounds(&bounds, &hasBounds, x, y);
    chunk.drawingBounds = bounds;
    chunk.hasBounds = hasBounds;
}

@implementation JKJDWGDrawing

- (instancetype)init {
    self = [super init];
    if (self) {
        _pathChunks = [NSMutableArray array];
        _openChunksByStyle = [NSMutableDictionary dictionary];
        _texts = [NSMutableArray array];
        _maskingPaths = [NSMutableArray array];
    }
    return self;
}

- (JKJDWGPathChunk *)chunkForNextEntityWithStyleKey:(NSString *)styleKey {
    if (self.styleWindowEntityCount >= JKJDWGEntitiesPerPathChunk) {
        [self.openChunksByStyle removeAllObjects];
        self.styleWindowEntityCount = 0;
    }
    JKJDWGPathChunk *chunk = self.openChunksByStyle[styleKey];
    if (!chunk) {
        chunk = [JKJDWGPathChunk new];
        chunk.styleKey = styleKey;
        self.openChunksByStyle[styleKey] = chunk;
        [self.pathChunks addObject:chunk];
    }
    chunk.entityCount++;
    self.styleWindowEntityCount++;
    self.currentChunk = chunk;
    return chunk;
}

- (JKJDWGPathChunk *)chunkForNextEntity {
    return [self chunkForNextEntityWithStyleKey:@"legacy"];
}

static void JKJDWGAppendTransformedPath(JKJDWGDrawing *drawing,
                                        CGPathRef sourcePath,
                                        CGAffineTransform transform,
                                        JKJDWGResolvedStyle style,
                                        CGRect *drawingBounds,
                                        BOOL *hasDrawingBounds) {
    CGMutablePathRef transformedPath = CGPathCreateMutable();
    CGPathAddPath(transformedPath, &transform, sourcePath);
    CGRect pathBounds = CGPathGetBoundingBox(transformedPath);
    if (style.geometricLineWidth > DBL_EPSILON &&
        !CGRectIsNull(pathBounds) && !CGRectIsInfinite(pathBounds)) {
        pathBounds = CGRectInset(pathBounds,
                                 -style.geometricLineWidth * .5,
                                 -style.geometricLineWidth * .5);
    }
    for (NSUInteger index = 0; index < style.clipCount; index++) {
        if (!style.clipPaths[index]) continue;
        pathBounds =
            CGRectIntersection(pathBounds,
                               CGPathGetBoundingBox(style.clipPaths[index]));
    }
    if (CGRectIsNull(pathBounds)) {
        CGPathRelease(transformedPath);
        return;
    }
    if (!JKJDWGRectIsSane(drawing, pathBounds)) {
        drawing.skippedInvalidEntityCount++;
        CGPathRelease(transformedPath);
        return;
    }
    JKJDWGPathChunk *chunk = [drawing chunkForNextEntityWithStyleKey:JKJDWGStyleKey(style)];
    if (chunk.entityCount == 1) {
        chunk.rgb = style.rgb;
        chunk.alpha = style.alpha;
        chunk.lineWidthPoints = style.lineWidthPoints;
        chunk.geometricLineWidth = style.geometricLineWidth;
        chunk.fillPath = style.fillPath;
        chunk.drawsPointMarkers = style.drawsPointMarkers;
        if (style.clipCount) {
            NSMutableArray<UIBezierPath *> *clippingPaths =
                [NSMutableArray arrayWithCapacity:style.clipCount];
            for (NSUInteger index = 0; index < style.clipCount; index++) {
                if (style.clipPaths[index]) {
                    UIBezierPath *clippingPath =
                        [UIBezierPath bezierPathWithCGPath:style.clipPaths[index]];
                    clippingPath.usesEvenOddFillRule =
                        (style.clipEvenOddMask & (1UL << index)) != 0;
                    [clippingPaths addObject:clippingPath];
                }
            }
            chunk.clippingPaths = clippingPaths;
        } else {
            chunk.clippingPaths = @[];
        }
        if (style.dashCount) {
            NSMutableArray<NSNumber *> *dashPattern =
                [NSMutableArray arrayWithCapacity:style.dashCount];
            for (NSUInteger index = 0; index < style.dashCount; index++) {
                [dashPattern addObject:@(style.dashLengths[index])];
            }
            chunk.dashPattern = dashPattern;
        } else {
            chunk.dashPattern = @[];
        }
    }
    CGPathAddPath(chunk.path, NULL, transformedPath);
    JKJDWGExpandBounds(drawingBounds, hasDrawingBounds, CGRectGetMinX(pathBounds), CGRectGetMinY(pathBounds));
    JKJDWGExpandBounds(drawingBounds, hasDrawingBounds, CGRectGetMaxX(pathBounds), CGRectGetMaxY(pathBounds));
    JKJDWGExpandChunkBounds(chunk, CGRectGetMinX(pathBounds), CGRectGetMinY(pathBounds));
    JKJDWGExpandChunkBounds(chunk, CGRectGetMaxX(pathBounds), CGRectGetMaxY(pathBounds));
    drawing.entityCount++;
    CGPathRelease(transformedPath);
}

static CGAffineTransform JKJDWGOCSTransform(BITCODE_BE extrusion,
                                            CGFloat elevation);

static CGAffineTransform JKJDWGInsertTransform(CGAffineTransform parent,
                                               BITCODE_3DPOINT insertionPoint,
                                               BITCODE_3DPOINT scale,
                                               BITCODE_BD rotation,
                                               BITCODE_BE extrusion,
                                               BITCODE_3DPOINT basePoint) {
    CGFloat scaleX = isfinite(scale.x) && fabs(scale.x) > DBL_EPSILON ? scale.x : 1;
    CGFloat scaleY = isfinite(scale.y) && fabs(scale.y) > DBL_EPSILON ? scale.y : 1;
    // Core Graphics applies row-vector transforms from left to right. A block
    // point must enter the INSERT's OCS before it enters its parent block.
    CGAffineTransform transform =
        CGAffineTransformConcat(
            JKJDWGOCSTransform(extrusion, insertionPoint.z), parent);
    transform = CGAffineTransformTranslate(transform, insertionPoint.x, insertionPoint.y);
    transform = CGAffineTransformRotate(transform, rotation);
    transform = CGAffineTransformScale(transform, scaleX, scaleY);
    return CGAffineTransformTranslate(transform, -basePoint.x, -basePoint.y);
}

static void JKJDWGAddBulgeSegment(CGMutablePathRef path,
                                  CGPoint start,
                                  CGPoint end,
                                  double bulge) {
    CGFloat deltaX = end.x - start.x;
    CGFloat deltaY = end.y - start.y;
    CGFloat chord = hypot(deltaX, deltaY);
    if (!isfinite(bulge) || fabs(bulge) < 1e-9 || chord < DBL_EPSILON) {
        CGPathAddLineToPoint(path, NULL, end.x, end.y);
        return;
    }

    CGFloat centerDistance = chord * (1.0 - bulge * bulge) / (4.0 * bulge);
    CGFloat middleX = (start.x + end.x) * 0.5;
    CGFloat middleY = (start.y + end.y) * 0.5;
    CGFloat centerX = middleX - deltaY * centerDistance / chord;
    CGFloat centerY = middleY + deltaX * centerDistance / chord;
    CGFloat radius = hypot(start.x - centerX, start.y - centerY);
    CGFloat startAngle = atan2(start.y - centerY, start.x - centerX);
    CGFloat endAngle = atan2(end.y - centerY, end.x - centerX);
    CGPathAddArc(path, NULL, centerX, centerY, radius,
                 startAngle, endAngle, bulge < 0);
}

typedef struct {
    CGFloat x;
    CGFloat y;
    CGFloat weight;
} JKJDWGCurveControlPoint;

static BOOL JKJDWGCurvePointIsFinite(CGPoint point) {
    return isfinite(point.x) && isfinite(point.y);
}

static BOOL JKJDWGEvaluateNURBS(const JKJDWGCurveControlPoint *controlPoints,
                                NSUInteger controlPointCount,
                                const BITCODE_BD *knots,
                                NSUInteger knotCount,
                                NSUInteger degree,
                                CGFloat parameter,
                                CGPoint *result) {
    if (!controlPoints || !knots || !result || degree == 0 ||
        controlPointCount <= degree ||
        knotCount < controlPointCount + degree + 1 ||
        degree > 32 || !isfinite(parameter)) {
        return NO;
    }

    NSUInteger span = controlPointCount - 1;
    CGFloat domainEnd = knots[controlPointCount];
    if (parameter < domainEnd) {
        span = degree;
        while (span + 1 < controlPointCount &&
               parameter >= knots[span + 1]) {
            span++;
        }
    }

    JKJDWGCurveControlPoint work[33] = {0};
    for (NSUInteger index = 0; index <= degree; index++) {
        JKJDWGCurveControlPoint point =
            controlPoints[span - degree + index];
        CGFloat weight = isfinite(point.weight) &&
                         fabs(point.weight) > DBL_EPSILON
            ? point.weight : 1;
        work[index] = (JKJDWGCurveControlPoint){
            point.x * weight, point.y * weight, weight
        };
    }

    for (NSUInteger level = 1; level <= degree; level++) {
        for (NSInteger index = (NSInteger)degree;
             index >= (NSInteger)level; index--) {
            NSUInteger knotIndex = span - degree + (NSUInteger)index;
            CGFloat denominator =
                knots[knotIndex + degree - level + 1] - knots[knotIndex];
            CGFloat alpha = fabs(denominator) > DBL_EPSILON
                ? (parameter - knots[knotIndex]) / denominator : 0;
            alpha = MIN(MAX(alpha, 0), 1);
            work[index].x =
                (1 - alpha) * work[index - 1].x + alpha * work[index].x;
            work[index].y =
                (1 - alpha) * work[index - 1].y + alpha * work[index].y;
            work[index].weight =
                (1 - alpha) * work[index - 1].weight +
                alpha * work[index].weight;
        }
    }

    CGFloat weight = work[degree].weight;
    if (!isfinite(weight) || fabs(weight) <= DBL_EPSILON) return NO;
    CGPoint point =
        CGPointMake(work[degree].x / weight, work[degree].y / weight);
    if (!JKJDWGCurvePointIsFinite(point)) return NO;
    *result = point;
    return YES;
}

static BOOL JKJDWGAppendNURBSCurve(CGMutablePathRef path,
                                   const JKJDWGCurveControlPoint *controlPoints,
                                   NSUInteger controlPointCount,
                                   const BITCODE_BD *knots,
                                   NSUInteger knotCount,
                                   NSUInteger degree,
                                   BOOL startsNewSubpath) {
    if (!path || !controlPoints || !knots || degree == 0 ||
        controlPointCount <= degree ||
        knotCount < controlPointCount + degree + 1) {
        return NO;
    }
    CGFloat start = knots[degree];
    CGFloat end = knots[controlPointCount];
    if (!isfinite(start) || !isfinite(end) || end - start <= DBL_EPSILON) {
        return NO;
    }

    NSUInteger sampleCount =
        MIN(MAX((controlPointCount - degree) * 12, 24), 768);
    BOOL appended = NO;
    for (NSUInteger index = 0; index <= sampleCount; index++) {
        CGFloat parameter =
            index == sampleCount
                ? end
                : start + (end - start) * index / sampleCount;
        CGPoint point;
        if (!JKJDWGEvaluateNURBS(controlPoints, controlPointCount, knots,
                                 knotCount, degree, parameter, &point)) {
            continue;
        }
        if (!appended) {
            if (startsNewSubpath) {
                CGPathMoveToPoint(path, NULL, point.x, point.y);
            } else {
                CGPathAddLineToPoint(path, NULL, point.x, point.y);
            }
            appended = YES;
        } else {
            CGPathAddLineToPoint(path, NULL, point.x, point.y);
        }
    }
    return appended;
}

static BOOL JKJDWGAppendFitPointCurve(CGMutablePathRef path,
                                      const CGPoint *points,
                                      NSUInteger pointCount,
                                      BOOL closed,
                                      BOOL startsNewSubpath) {
    if (!path || !points || pointCount < 2) return NO;
    CGPoint first = points[0];
    if (!JKJDWGCurvePointIsFinite(first)) return NO;
    if (startsNewSubpath) {
        CGPathMoveToPoint(path, NULL, first.x, first.y);
    } else {
        CGPathAddLineToPoint(path, NULL, first.x, first.y);
    }

    NSUInteger segmentCount = closed ? pointCount : pointCount - 1;
    for (NSUInteger index = 0; index < segmentCount; index++) {
        CGPoint previous = points[index == 0
            ? (closed ? pointCount - 1 : 0) : index - 1];
        CGPoint start = points[index];
        CGPoint end = points[(index + 1) % pointCount];
        CGPoint following = points[index + 2 < pointCount
            ? index + 2 : (closed ? (index + 2) % pointCount : pointCount - 1)];
        if (!JKJDWGCurvePointIsFinite(previous) ||
            !JKJDWGCurvePointIsFinite(start) ||
            !JKJDWGCurvePointIsFinite(end) ||
            !JKJDWGCurvePointIsFinite(following)) {
            continue;
        }
        CGPoint control1 =
            CGPointMake(start.x + (end.x - previous.x) / 6.0,
                        start.y + (end.y - previous.y) / 6.0);
        CGPoint control2 =
            CGPointMake(end.x - (following.x - start.x) / 6.0,
                        end.y - (following.y - start.y) / 6.0);
        CGPathAddCurveToPoint(path, NULL,
                              control1.x, control1.y,
                              control2.x, control2.y,
                              end.x, end.y);
    }
    if (closed) CGPathCloseSubpath(path);
    return YES;
}

static BOOL JKJDWGHatchSegmentTouchesBoundary(CGPathRef boundaryPath,
                                               CGPoint start,
                                               CGPoint end) {
    if (!boundaryPath) return YES;
    // Dashed hatch definitions can create thousands of segments in the empty
    // space between disjoint loops. Test several positions along each dash so
    // those segments never enter the Core Graphics path in the first place.
    static const CGFloat samples[] = {0, .25, .5, .75, 1};
    for (NSUInteger index = 0;
         index < sizeof(samples) / sizeof(samples[0]); index++) {
        CGFloat amount = samples[index];
        CGPoint point = CGPointMake(start.x + (end.x - start.x) * amount,
                                    start.y + (end.y - start.y) * amount);
        if (CGPathContainsPoint(boundaryPath, NULL, point, true)) return YES;
    }
    return NO;
}

static BOOL JKJDWGHatchPathIsDegenerate(Dwg_HATCH_Path *hatchPath) {
    if (!hatchPath || !hatchPath->segs ||
        hatchPath->num_segs_or_paths < 4 || (hatchPath->flag & 2) ||
        !(hatchPath->flag & 4)) {
        return NO;
    }
    CGFloat characteristicLength = 0;
    for (BITCODE_BL index = 0;
         index < hatchPath->num_segs_or_paths; index++) {
        Dwg_HATCH_PathSeg *segment = &hatchPath->segs[index];
        CGFloat length = 0;
        if (segment->curve_type == 1) {
            length = hypot(
                segment->second_endpoint.x - segment->first_endpoint.x,
                segment->second_endpoint.y - segment->first_endpoint.y);
        } else if (segment->curve_type == 2) {
            length = fabs(segment->radius);
        } else if (segment->curve_type == 3) {
            length = hypot(segment->endpoint.x, segment->endpoint.y);
        }
        if (isfinite(length)) {
            characteristicLength = MAX(characteristicLength, length);
        }
    }
    // Boundary coordinates decoded from DWG are not bit-exact: edges that
    // AutoCAD treats as zero-length can still be a few 1e-5 units long.
    CGFloat collapsedLength = MAX(characteristicLength * 1e-6, 1e-9);
    BITCODE_BL degenerateSegmentCount = 0;
    for (BITCODE_BL index = 0;
         index < hatchPath->num_segs_or_paths; index++) {
        Dwg_HATCH_PathSeg *segment = &hatchPath->segs[index];
        BOOL degenerate = NO;
        if (segment->curve_type == 1) {
            degenerate =
                hypot(segment->second_endpoint.x - segment->first_endpoint.x,
                      segment->second_endpoint.y - segment->first_endpoint.y) <=
                collapsedLength;
        } else if (segment->curve_type == 2) {
            degenerate = !isfinite(segment->radius) ||
                         segment->radius <= 1e-9;
        } else if (segment->curve_type == 3) {
            CGFloat majorRadius =
                hypot(segment->endpoint.x, segment->endpoint.y);
            degenerate = !isfinite(majorRadius) || majorRadius <= 1e-9 ||
                         !isfinite(segment->minor_major_ratio) ||
                         segment->minor_major_ratio <= 1e-9;
        } else if (segment->curve_type == 4) {
            degenerate = segment->num_control_points < 2 &&
                         segment->num_fitpts < 2;
        }
        if (degenerate) degenerateSegmentCount++;
    }
    // LibreDWG can expose a corrupt derived island as a mixture of valid
    // curves and repeated collapsed edges. Treating that as a closed loop
    // changes hatch parity and fills an area AutoCAD leaves empty.
    return degenerateSegmentCount * 2 >= hatchPath->num_segs_or_paths;
}

typedef struct {
    CGPoint startPoints[2];
    CGPoint endPoints[2];
    NSUInteger optionCount;
} JKJDWGHatchSegmentOptions;

static BOOL JKJDWGGetHatchSegmentOptions(
    Dwg_HATCH_PathSeg *segment, JKJDWGHatchSegmentOptions *options) {
    if (!segment || !options) return NO;
    *options = (JKJDWGHatchSegmentOptions){0};
    if (segment->curve_type == 1) {
        options->startPoints[0] =
            CGPointMake(segment->first_endpoint.x,
                        segment->first_endpoint.y);
        options->endPoints[0] =
            CGPointMake(segment->second_endpoint.x,
                        segment->second_endpoint.y);
        options->optionCount = 1;
    } else if (segment->curve_type == 2 &&
               isfinite(segment->radius) && segment->radius > 0) {
        CGFloat angleSign = segment->is_ccw ? 1.0 : -1.0;
        CGFloat startAngle = angleSign * segment->start_angle;
        CGFloat endAngle = angleSign * segment->end_angle;
        options->startPoints[0] = CGPointMake(
            segment->center.x + cos(startAngle) * segment->radius,
            segment->center.y + sin(startAngle) * segment->radius);
        options->endPoints[0] = CGPointMake(
            segment->center.x + cos(endAngle) * segment->radius,
            segment->center.y + sin(endAngle) * segment->radius);
        options->optionCount = 1;
    } else if (segment->curve_type == 3) {
        CGFloat majorRadius =
            hypot(segment->endpoint.x, segment->endpoint.y);
        if (!isfinite(majorRadius) || majorRadius <= 0 ||
            !isfinite(segment->minor_major_ratio) ||
            segment->minor_major_ratio <= 0) {
            return NO;
        }
        CGAffineTransform ellipseTransform =
            CGAffineTransformMakeTranslation(segment->center.x,
                                               segment->center.y);
        ellipseTransform = CGAffineTransformRotate(
            ellipseTransform,
            atan2(segment->endpoint.y, segment->endpoint.x));
        ellipseTransform = CGAffineTransformScale(
            ellipseTransform, majorRadius,
            majorRadius * segment->minor_major_ratio);
        options->startPoints[0] = CGPointApplyAffineTransform(
            CGPointMake(cos(segment->start_angle),
                        sin(segment->start_angle)),
            ellipseTransform);
        options->endPoints[0] = CGPointApplyAffineTransform(
            CGPointMake(cos(segment->end_angle),
                        sin(segment->end_angle)),
            ellipseTransform);
        options->startPoints[1] = CGPointApplyAffineTransform(
            CGPointMake(cos(-segment->start_angle),
                        sin(-segment->start_angle)),
            ellipseTransform);
        options->endPoints[1] = CGPointApplyAffineTransform(
            CGPointMake(cos(-segment->end_angle),
                        sin(-segment->end_angle)),
            ellipseTransform);
        options->optionCount = 2;
    }
    if (options->optionCount == 0 || options->optionCount > 2) {
        return NO;
    }
    for (NSUInteger index = 0; index < options->optionCount; index++) {
        if (!JKJDWGCurvePointIsFinite(options->startPoints[index]) ||
            !JKJDWGCurvePointIsFinite(options->endPoints[index])) {
            return NO;
        }
    }
    return YES;
}

static CGFloat JKJDWGHatchPointGap(CGPoint first, CGPoint second) {
    return hypot(first.x - second.x, first.y - second.y);
}

static uint8_t *JKJDWGCreateHatchEllipseSelections(
    Dwg_HATCH_Path *hatchPath) {
    if (!hatchPath || !hatchPath->segs || (hatchPath->flag & 2) ||
        hatchPath->num_segs_or_paths == 0) {
        return NULL;
    }
    NSUInteger count = (NSUInteger)hatchPath->num_segs_or_paths;
    BOOL containsEllipse = NO;
    for (NSUInteger index = 0; index < count; index++) {
        BITCODE_RC curveType = hatchPath->segs[index].curve_type;
        if (curveType < 1 || curveType > 3) return NULL;
        containsEllipse = containsEllipse || curveType == 3;
    }
    if (!containsEllipse || count > NSUIntegerMax / 2) return NULL;

    JKJDWGHatchSegmentOptions *options =
        calloc(count, sizeof(JKJDWGHatchSegmentOptions));
    uint8_t *selections = calloc(count, sizeof(uint8_t));
    uint8_t *predecessors = calloc(count * 2, sizeof(uint8_t));
    if (!options || !selections || !predecessors) {
        free(predecessors);
        free(selections);
        free(options);
        return NULL;
    }

    for (NSUInteger index = 0; index < count; index++) {
        if (!JKJDWGGetHatchSegmentOptions(&hatchPath->segs[index],
                                          &options[index])) {
            free(predecessors);
            free(selections);
            free(options);
            return NULL;
        }
    }

    // LibreDWG can mirror individual ellipse-edge angles around the local
    // major axis. Select all orientations together: a locally closest start
    // point can still force the final edge to close across a large false loop.
    CGFloat bestCost = INFINITY;
    for (uint8_t firstState = 0;
         firstState < options[0].optionCount; firstState++) {
        CGFloat previousCosts[2] = {INFINITY, INFINITY};
        previousCosts[firstState] = 0;
        for (NSUInteger index = 1; index < count; index++) {
            CGFloat nextCosts[2] = {INFINITY, INFINITY};
            for (uint8_t state = 0;
                 state < options[index].optionCount; state++) {
                for (uint8_t previousState = 0;
                     previousState < options[index - 1].optionCount;
                     previousState++) {
                    CGFloat candidateCost = previousCosts[previousState] +
                        JKJDWGHatchPointGap(
                            options[index - 1].endPoints[previousState],
                            options[index].startPoints[state]);
                    if (candidateCost < nextCosts[state]) {
                        nextCosts[state] = candidateCost;
                        predecessors[index * 2 + state] = previousState;
                    }
                }
            }
            previousCosts[0] = nextCosts[0];
            previousCosts[1] = nextCosts[1];
        }
        for (uint8_t lastState = 0;
             lastState < options[count - 1].optionCount; lastState++) {
            CGFloat candidateCost = previousCosts[lastState];
            if (!(hatchPath->flag & 0x20)) {
                candidateCost += JKJDWGHatchPointGap(
                    options[count - 1].endPoints[lastState],
                    options[0].startPoints[firstState]);
            }
            if (candidateCost < bestCost) {
                selections[count - 1] = lastState;
                for (NSUInteger index = count - 1; index > 0; index--) {
                    selections[index - 1] =
                        predecessors[index * 2 +
                                     selections[index]];
                }
                bestCost = candidateCost;
            }
        }
    }

    free(predecessors);
    free(options);
    if (!isfinite(bestCost)) {
        free(selections);
        return NULL;
    }
    return selections;
}

static CGMutablePathRef JKJDWGCreateHatchPatternPathPass(
    Dwg_Entity_HATCH *hatch, CGPathRef boundaryPath,
    BOOL filtersToBoundary) {
    CGMutablePathRef patternPath = CGPathCreateMutable();
    CGRect boundaryBounds = boundaryPath
        ? CGPathGetBoundingBox(boundaryPath) : CGRectNull;
    if (!hatch || !hatch->deflines || hatch->num_deflines == 0 ||
        CGRectIsNull(boundaryBounds) || CGRectIsInfinite(boundaryBounds)) {
        return patternPath;
    }

    CGPoint corners[] = {
        CGPointMake(CGRectGetMinX(boundaryBounds), CGRectGetMinY(boundaryBounds)),
        CGPointMake(CGRectGetMaxX(boundaryBounds), CGRectGetMinY(boundaryBounds)),
        CGPointMake(CGRectGetMaxX(boundaryBounds), CGRectGetMaxY(boundaryBounds)),
        CGPointMake(CGRectGetMinX(boundaryBounds), CGRectGetMaxY(boundaryBounds))
    };
    NSUInteger emittedSegments = 0;
    // A single dense HATCH can expand into hundreds of thousands of path
    // elements and make Core Graphics drop later entities. Empty-space dashes
    // are filtered below; retain this final guard for pathological patterns.
    const NSUInteger maximumSegments = 2000;
    for (BITCODE_BS lineIndex = 0;
         lineIndex < hatch->num_deflines && emittedSegments < maximumSegments;
         lineIndex++) {
        Dwg_HATCH_DefLine *definition = &hatch->deflines[lineIndex];
        if (!isfinite(definition->angle) ||
            !isfinite(definition->pt0.x) ||
            !isfinite(definition->pt0.y) ||
            !isfinite(definition->offset.x) ||
            !isfinite(definition->offset.y)) {
            continue;
        }
        CGPoint direction = CGPointMake(cos(definition->angle),
                                        sin(definition->angle));
        CGPoint normal = CGPointMake(-direction.y, direction.x);
        CGFloat spacing = definition->offset.x * normal.x +
                          definition->offset.y * normal.y;
        if (!isfinite(spacing) || fabs(spacing) <= DBL_EPSILON) continue;

        CGFloat minimumProjection = CGFLOAT_MAX;
        CGFloat maximumProjection = -CGFLOAT_MAX;
        for (NSUInteger cornerIndex = 0; cornerIndex < 4; cornerIndex++) {
            CGFloat projection = corners[cornerIndex].x * normal.x +
                                 corners[cornerIndex].y * normal.y;
            minimumProjection = MIN(minimumProjection, projection);
            maximumProjection = MAX(maximumProjection, projection);
        }
        CGFloat baseProjection = definition->pt0.x * normal.x +
                                 definition->pt0.y * normal.y;
        CGFloat firstValue = (minimumProjection - baseProjection) / spacing;
        CGFloat lastValue = (maximumProjection - baseProjection) / spacing;
        NSInteger firstLine = (NSInteger)floor(MIN(firstValue, lastValue)) - 1;
        NSInteger lastLine = (NSInteger)ceil(MAX(firstValue, lastValue)) + 1;
        if (lastLine < firstLine || lastLine - firstLine > 4096) continue;

        CGFloat dashCycle = 0;
        BITCODE_BS dashCount = definition->dashes
            ? definition->num_dashes
            : 0;
        for (BITCODE_BS dashIndex = 0;
             dashIndex < dashCount; dashIndex++) {
            if (isfinite(definition->dashes[dashIndex])) {
                dashCycle += fabs(definition->dashes[dashIndex]);
            }
        }
        for (NSInteger repeatedLine = firstLine;
             repeatedLine <= lastLine && emittedSegments < maximumSegments;
             repeatedLine++) {
            CGPoint lineBase = CGPointMake(
                definition->pt0.x + repeatedLine * definition->offset.x,
                definition->pt0.y + repeatedLine * definition->offset.y);
            CGFloat minimumParameter = CGFLOAT_MAX;
            CGFloat maximumParameter = -CGFLOAT_MAX;
            for (NSUInteger cornerIndex = 0; cornerIndex < 4; cornerIndex++) {
                CGFloat parameter =
                    (corners[cornerIndex].x - lineBase.x) * direction.x +
                    (corners[cornerIndex].y - lineBase.y) * direction.y;
                minimumParameter = MIN(minimumParameter, parameter);
                maximumParameter = MAX(maximumParameter, parameter);
            }
            if (dashCount == 0 || dashCycle <= DBL_EPSILON) {
                CGPathMoveToPoint(patternPath, NULL,
                                  lineBase.x + direction.x * minimumParameter,
                                  lineBase.y + direction.y * minimumParameter);
                CGPathAddLineToPoint(patternPath, NULL,
                                     lineBase.x + direction.x * maximumParameter,
                                     lineBase.y + direction.y * maximumParameter);
                emittedSegments++;
                continue;
            }

            CGFloat cycleStart = floor(minimumParameter / dashCycle) * dashCycle;
            for (; cycleStart <= maximumParameter &&
                   emittedSegments < maximumSegments;
                 cycleStart += dashCycle) {
                CGFloat cursor = cycleStart;
                for (BITCODE_BS dashIndex = 0;
                     dashIndex < dashCount &&
                     emittedSegments < maximumSegments;
                     dashIndex++) {
                    CGFloat dash = definition->dashes[dashIndex];
                    if (!isfinite(dash)) continue;
                    CGFloat length = fabs(dash);
                    if (dash > DBL_EPSILON) {
                        CGFloat segmentStart = MAX(cursor, minimumParameter);
                        CGFloat segmentEnd =
                            MIN(cursor + length, maximumParameter);
                        if (segmentEnd > segmentStart) {
                            CGPoint startPoint = CGPointMake(
                                lineBase.x + direction.x * segmentStart,
                                lineBase.y + direction.y * segmentStart);
                            CGPoint endPoint = CGPointMake(
                                lineBase.x + direction.x * segmentEnd,
                                lineBase.y + direction.y * segmentEnd);
                            if (filtersToBoundary &&
                                !JKJDWGHatchSegmentTouchesBoundary(
                                    boundaryPath, startPoint, endPoint)) {
                                cursor += length;
                                continue;
                            }
                            CGPathMoveToPoint(
                                patternPath, NULL, startPoint.x, startPoint.y);
                            CGPathAddLineToPoint(
                                patternPath, NULL, endPoint.x, endPoint.y);
                            emittedSegments++;
                        }
                    } else if (fabs(dash) <= DBL_EPSILON &&
                               cursor >= minimumParameter &&
                               cursor <= maximumParameter) {
                        CGPoint dotCenter = CGPointMake(
                            lineBase.x + direction.x * cursor,
                            lineBase.y + direction.y * cursor);
                        if (filtersToBoundary &&
                            !CGPathContainsPoint(boundaryPath, NULL,
                                                 dotCenter, true)) {
                            cursor += length;
                            continue;
                        }
                        CGFloat dotLength = MAX(
                            MIN(fabs(spacing) * .08, dashCycle * .02), .25);
                        CGFloat halfDot = dotLength * .5;
                        CGPathMoveToPoint(
                            patternPath, NULL,
                            dotCenter.x - direction.x * halfDot,
                            dotCenter.y - direction.y * halfDot);
                        CGPathAddLineToPoint(
                            patternPath, NULL,
                            dotCenter.x + direction.x * halfDot,
                            dotCenter.y + direction.y * halfDot);
                        emittedSegments++;
                    }
                    cursor += length;
                }
            }
        }
    }
    if (emittedSegments >= maximumSegments) {
        CGPathRelease(patternPath);
        return CGPathCreateMutable();
    }
    return patternPath;
}

static CGMutablePathRef JKJDWGCreateHatchPatternPath(
    Dwg_Entity_HATCH *hatch, CGPathRef boundaryPath) {
    // A single non-rectangular loop can still leave large empty corners in
    // its bounding box. Filtering every dashed pattern prevents those empty
    // segments from exhausting the path guard and discarding the whole hatch.
    return JKJDWGCreateHatchPatternPathPass(hatch, boundaryPath, YES);
}

static CGAffineTransform JKJDWGOCSTransform(BITCODE_BE extrusion,
                                            CGFloat elevation) {
    CGFloat nx = isfinite(extrusion.x) ? extrusion.x : 0;
    CGFloat ny = isfinite(extrusion.y) ? extrusion.y : 0;
    CGFloat nz = isfinite(extrusion.z) ? extrusion.z : 1;
    CGFloat normalLength = sqrt(nx * nx + ny * ny + nz * nz);
    if (normalLength < DBL_EPSILON) {
        nx = 0;
        ny = 0;
        nz = 1;
    } else {
        nx /= normalLength;
        ny /= normalLength;
        nz /= normalLength;
    }

    CGFloat ax, ay, az;
    if (fabs(nx) < (1.0 / 64.0) && fabs(ny) < (1.0 / 64.0)) {
        ax = nz;
        ay = 0;
        az = -nx;
    } else {
        ax = -ny;
        ay = nx;
        az = 0;
    }
    CGFloat axisLength = sqrt(ax * ax + ay * ay + az * az);
    if (axisLength < DBL_EPSILON) {
        ax = 1;
        ay = 0;
        az = 0;
    } else {
        ax /= axisLength;
        ay /= axisLength;
        az /= axisLength;
    }
    CGFloat bx = ny * az - nz * ay;
    CGFloat by = nz * ax - nx * az;
    return CGAffineTransformMake(ax, ay, bx, by,
                                 elevation * nx, elevation * ny);
}

static BOOL JKJDWGEllipseTransform(Dwg_Entity_ELLIPSE *ellipse,
                                   CGAffineTransform *transform) {
    if (!ellipse || !transform || !isfinite(ellipse->axis_ratio) ||
        ellipse->axis_ratio <= DBL_EPSILON) {
        return NO;
    }

    CGFloat majorX = ellipse->sm_axis.x;
    CGFloat majorY = ellipse->sm_axis.y;
    CGFloat majorZ = ellipse->sm_axis.z;
    CGFloat majorLength = sqrt(majorX * majorX +
                               majorY * majorY +
                               majorZ * majorZ);
    if (!isfinite(majorLength) || majorLength <= DBL_EPSILON) return NO;

    CGFloat normalX = isfinite(ellipse->extrusion.x) ? ellipse->extrusion.x : 0;
    CGFloat normalY = isfinite(ellipse->extrusion.y) ? ellipse->extrusion.y : 0;
    CGFloat normalZ = isfinite(ellipse->extrusion.z) ? ellipse->extrusion.z : 1;
    CGFloat normalLength = sqrt(normalX * normalX +
                                normalY * normalY +
                                normalZ * normalZ);
    if (normalLength <= DBL_EPSILON) {
        normalX = 0;
        normalY = 0;
        normalZ = 1;
    } else {
        normalX /= normalLength;
        normalY /= normalLength;
        normalZ /= normalLength;
    }

    // ELLIPSE stores its center and major-axis endpoint in WCS. Derive the
    // minor axis from the entity normal so a negative extrusion reverses the
    // parameter direction instead of mirroring partial ellipses into space.
    CGFloat minorX = normalY * majorZ - normalZ * majorY;
    CGFloat minorY = normalZ * majorX - normalX * majorZ;
    CGFloat minorZ = normalX * majorY - normalY * majorX;
    CGFloat minorLength = sqrt(minorX * minorX +
                               minorY * minorY +
                               minorZ * minorZ);
    if (!isfinite(minorLength) || minorLength <= DBL_EPSILON) return NO;
    CGFloat minorScale = majorLength * ellipse->axis_ratio / minorLength;
    minorX *= minorScale;
    minorY *= minorScale;

    if (!isfinite(ellipse->center.x) || !isfinite(ellipse->center.y) ||
        !isfinite(majorX) || !isfinite(majorY) ||
        !isfinite(minorX) || !isfinite(minorY)) {
        return NO;
    }
    *transform = CGAffineTransformMake(majorX, majorY,
                                       minorX, minorY,
                                       ellipse->center.x,
                                       ellipse->center.y);
    return YES;
}

static CGFloat JKJDWGPointMarkerSize(Dwg_Data *dwg) {
    if (!dwg) return 1;
    CGFloat configuredSize = dwg->header_vars.PDSIZE;
    if (isfinite(configuredSize) && configuredSize > DBL_EPSILON) {
        return configuredSize * .5;
    }
    CGFloat width =
        fabs(dwg->header_vars.EXTMAX.x - dwg->header_vars.EXTMIN.x);
    CGFloat height =
        fabs(dwg->header_vars.EXTMAX.y - dwg->header_vars.EXTMIN.y);
    CGFloat extent = MAX(width, height);
    if (!isfinite(extent) || extent <= DBL_EPSILON) return 1;
    if (configuredSize < -DBL_EPSILON) {
        return MAX(extent * fabs(configuredSize) / 200.0, extent * 1e-6);
    }
    // PDSIZE == 0 is viewport-relative in AutoCAD. Keep a conservative
    // model-space approximation; unlike the previous hard-coded cross it will
    // not overwhelm dense point clouds.
    return MAX(extent * 0.00005, extent * 1e-6);
}

static CGFloat JKJDWGLWPolylineGeometricWidth(Dwg_Entity_LWPOLYLINE *polyline) {
    if (!polyline) return 0;
    CGFloat constantWidth =
        isfinite(polyline->const_width) ? fabs(polyline->const_width) : 0;
    if (constantWidth > DBL_EPSILON) return constantWidth;
    CGFloat width = 0;
    BOOL hasWidth = NO;
    BOOL hasVariableWidth = NO;
    if (polyline->widths) {
        for (BITCODE_BL index = 0; index < polyline->num_widths; index++) {
            CGFloat widths[] = {
                fabs(polyline->widths[index].start),
                fabs(polyline->widths[index].end)
            };
            for (NSUInteger widthIndex = 0; widthIndex < 2; widthIndex++) {
                if (!isfinite(widths[widthIndex])) continue;
                if (!hasWidth) {
                    width = widths[widthIndex];
                    hasWidth = YES;
                } else if (fabs(widths[widthIndex] - width) >
                           MAX(width, 1) * 1e-6) {
                    hasVariableWidth = YES;
                }
            }
        }
    }
    return hasWidth && !hasVariableWidth ? width : 0;
}

static CGFloat JKJDWGPolyline2DGeometricWidth(Dwg_Object *polylineObject,
                                               Dwg_Entity_POLYLINE_2D *polyline) {
    if (!polylineObject || !polyline) return 0;
    CGFloat startWidth = fabs(polyline->start_width);
    CGFloat endWidth = fabs(polyline->end_width);
    BOOL hasWidth = (isfinite(startWidth) && startWidth > DBL_EPSILON) ||
                    (isfinite(endWidth) && endWidth > DBL_EPSILON);
    CGFloat width = startWidth > DBL_EPSILON ? startWidth : endWidth;
    BOOL hasVariableWidth =
        hasWidth && startWidth > DBL_EPSILON && endWidth > DBL_EPSILON &&
        fabs(endWidth - startWidth) > MAX(width, 1) * 1e-6;
    Dwg_Object *vertexObject = get_first_owned_subentity(polylineObject);
    while (vertexObject) {
        if (vertexObject->fixedtype == DWG_TYPE_VERTEX_2D &&
            vertexObject->tio.entity && vertexObject->tio.entity->tio.VERTEX_2D) {
            Dwg_Entity_VERTEX_2D *vertex = vertexObject->tio.entity->tio.VERTEX_2D;
            CGFloat widths[] = {fabs(vertex->start_width), fabs(vertex->end_width)};
            if (isfinite(widths[0]) && isfinite(widths[1]) &&
                ((widths[0] <= DBL_EPSILON) !=
                 (widths[1] <= DBL_EPSILON))) {
                hasVariableWidth = YES;
            }
            for (NSUInteger widthIndex = 0; widthIndex < 2; widthIndex++) {
                if (!isfinite(widths[widthIndex]) ||
                    widths[widthIndex] <= DBL_EPSILON) continue;
                if (!hasWidth) {
                    width = widths[widthIndex];
                    hasWidth = YES;
                } else if (fabs(widths[widthIndex] - width) >
                           MAX(width, 1) * 1e-6) {
                    hasVariableWidth = YES;
                }
            }
        }
        vertexObject = get_next_owned_subentity(polylineObject, vertexObject);
    }
    return hasWidth && !hasVariableWidth ? width : 0;
}

static Dwg_Object *JKJDWGFindSpatialFilterObject(Dwg_Data *dwg,
                                                  Dwg_Object *object,
                                                  NSUInteger depth) {
    if (!dwg || !object || depth > 4) return NULL;
    if (object->fixedtype == DWG_TYPE_SPATIAL_FILTER &&
        object->tio.object && object->tio.object->tio.SPATIAL_FILTER) {
        return object;
    }
    if ((object->fixedtype != DWG_TYPE_DICTIONARY &&
         object->fixedtype != DWG_TYPE_DICTIONARYWDFLT) ||
        !object->tio.object) {
        return NULL;
    }
    Dwg_Object_DICTIONARY *dictionary =
        object->fixedtype == DWG_TYPE_DICTIONARY
            ? object->tio.object->tio.DICTIONARY
            : (Dwg_Object_DICTIONARY *)object->tio.object->tio.DICTIONARYWDFLT;
    if (!dictionary || !dictionary->itemhandles) return NULL;
    for (BITCODE_BL index = 0; index < dictionary->numitems; index++) {
        Dwg_Object *item = dwg_ref_object(dwg, dictionary->itemhandles[index]);
        Dwg_Object *spatialFilter =
            JKJDWGFindSpatialFilterObject(dwg, item, depth + 1);
        if (spatialFilter) return spatialFilter;
    }
    return NULL;
}

static Dwg_Object *JKJDWGSpatialFilterObjectForEntity(Dwg_Data *dwg,
                                                       Dwg_Object_Entity *entity) {
    if (!dwg || !entity || !entity->xdicobjhandle) return NULL;
    Dwg_Object *extensionDictionary =
        dwg_ref_object(dwg, entity->xdicobjhandle);
    return JKJDWGFindSpatialFilterObject(dwg, extensionDictionary, 0);
}

static BOOL JKJDWGAffineTransformFromSpatialMatrix(const BITCODE_BD *matrix,
                                                    CGAffineTransform *transform) {
    if (!matrix || !transform) return NO;
    CGFloat a = matrix[0];
    CGFloat b = matrix[1];
    CGFloat c = matrix[3];
    CGFloat d = matrix[4];
    CGFloat tx = matrix[9];
    CGFloat ty = matrix[10];
    CGFloat determinant = a * d - b * c;
    if (!isfinite(a) || !isfinite(b) || !isfinite(c) || !isfinite(d) ||
        !isfinite(tx) || !isfinite(ty) || fabs(determinant) < DBL_EPSILON) {
        return NO;
    }
    *transform = CGAffineTransformMake(a, b, c, d, tx, ty);
    return YES;
}

static CGPathRef JKJDWGCreateSpatialClipPath(Dwg_Object_SPATIAL_FILTER *filter,
                                             CGAffineTransform blockTransform) {
    if (!filter || !filter->clip_verts || filter->num_clip_verts < 2) return NULL;
    CGMutablePathRef localPath = CGPathCreateMutable();
    if (filter->num_clip_verts == 2) {
        BITCODE_2RD first = filter->clip_verts[0];
        BITCODE_2RD second = filter->clip_verts[1];
        CGRect rectangle =
            CGRectMake(MIN(first.x, second.x), MIN(first.y, second.y),
                       fabs(second.x - first.x), fabs(second.y - first.y));
        CGPathAddRect(localPath, NULL, rectangle);
    } else {
        CGPathMoveToPoint(localPath, NULL,
                          filter->clip_verts[0].x, filter->clip_verts[0].y);
        for (BITCODE_BS index = 1; index < filter->num_clip_verts; index++) {
            CGPathAddLineToPoint(localPath, NULL,
                                 filter->clip_verts[index].x,
                                 filter->clip_verts[index].y);
        }
        CGPathCloseSubpath(localPath);
    }

    CGAffineTransform clipTransform;
    CGAffineTransform storedInverseBlockTransform;
    if (JKJDWGAffineTransformFromSpatialMatrix(filter->inverse_transform,
                                                &storedInverseBlockTransform)) {
        clipTransform = CGAffineTransformInvert(storedInverseBlockTransform);
    } else {
        CGAffineTransform clipOCS =
            JKJDWGOCSTransform(filter->extrusion, filter->origin.z);
        clipOCS.tx += filter->origin.x;
        clipOCS.ty += filter->origin.y;
        clipTransform = CGAffineTransformConcat(clipOCS, blockTransform);
    }
    CGMutablePathRef worldPath = CGPathCreateMutable();
    CGPathAddPath(worldPath, &clipTransform, localPath);
    CGPathRelease(localPath);
    return worldPath;
}

static void JKJDWGRenderBlockObject(Dwg_Data *dwg,
                                    Dwg_Object *object,
                                    JKJDWGDrawing *drawing,
                                    CGAffineTransform transform,
                                    JKJDWGResolvedStyle inheritedStyle,
                                    CGRect *drawingBounds,
                                    BOOL *hasDrawingBounds,
                                    NSMutableSet<NSNumber *> *blockStack,
                                    NSUInteger depth);

static Dwg_DIMENSION_common *JKJDWGDimensionCommon(Dwg_Object *object) {
    if (!object || object->supertype != DWG_SUPERTYPE_ENTITY ||
        !object->tio.entity) {
        return NULL;
    }
    switch (object->fixedtype) {
        case DWG_TYPE_DIMENSION_ORDINATE:
        case DWG_TYPE_DIMENSION_LINEAR:
        case DWG_TYPE_DIMENSION_ALIGNED:
        case DWG_TYPE_DIMENSION_ANG3PT:
        case DWG_TYPE_DIMENSION_ANG2LN:
        case DWG_TYPE_DIMENSION_RADIUS:
        case DWG_TYPE_DIMENSION_DIAMETER:
        case DWG_TYPE_ARC_DIMENSION:
        case DWG_TYPE_LARGE_RADIAL_DIMENSION:
            return object->tio.entity->tio.DIMENSION_common;
        default:
            return NULL;
    }
}

static Dwg_Object_DIMSTYLE *JKJDWGLeaderDimensionStyle(
    Dwg_Data *dwg,
    Dwg_Entity_LEADER *leader) {
    if (!dwg || !leader || !leader->dimstyle) return NULL;
    Dwg_Object *styleObject = dwg_ref_object(dwg, leader->dimstyle);
    if (!styleObject || styleObject->fixedtype != DWG_TYPE_DIMSTYLE ||
        !styleObject->tio.object || !styleObject->tio.object->tio.DIMSTYLE) {
        return NULL;
    }
    return styleObject->tio.object->tio.DIMSTYLE;
}

static CGFloat JKJDWGLeaderArrowSize(Dwg_Data *dwg,
                                     Dwg_Entity_LEADER *leader,
                                     Dwg_Object_DIMSTYLE *dimensionStyle) {
    if (leader && isfinite(leader->dimasz) &&
        fabs(leader->dimasz) > DBL_EPSILON) {
        return fabs(leader->dimasz);
    }
    if (dimensionStyle && isfinite(dimensionStyle->DIMASZ) &&
        fabs(dimensionStyle->DIMASZ) > DBL_EPSILON) {
        CGFloat dimensionScale =
            isfinite(dimensionStyle->DIMSCALE) &&
            fabs(dimensionStyle->DIMSCALE) > DBL_EPSILON
                ? fabs(dimensionStyle->DIMSCALE) : 1;
        return fabs(dimensionStyle->DIMASZ) * dimensionScale;
    }
    if (dwg && isfinite(dwg->header_vars.DIMASZ) &&
        fabs(dwg->header_vars.DIMASZ) > DBL_EPSILON) {
        CGFloat dimensionScale =
            isfinite(dwg->header_vars.DIMSCALE) &&
            fabs(dwg->header_vars.DIMSCALE) > DBL_EPSILON
                ? fabs(dwg->header_vars.DIMSCALE) : 1;
        return fabs(dwg->header_vars.DIMASZ) * dimensionScale;
    }
    if (leader && isfinite(leader->box_height) &&
        fabs(leader->box_height) > DBL_EPSILON) {
        return fabs(leader->box_height) * 0.5;
    }
    return 0;
}

static BITCODE_H JKJDWGLeaderArrowBlock(
    Dwg_Data *dwg,
    Dwg_Object_DIMSTYLE *dimensionStyle) {
    if (dimensionStyle && dimensionStyle->DIMLDRBLK) {
        return dimensionStyle->DIMLDRBLK;
    }
    return dwg ? dwg->header_vars.DIMLDRBLK : NULL;
}

static CGFloat JKJDWGArrowBlockFilledDotRadiusScale(
    Dwg_Object *arrowBlockObject) {
    if (!arrowBlockObject ||
        arrowBlockObject->fixedtype != DWG_TYPE_BLOCK_HEADER ||
        !arrowBlockObject->tio.object ||
        !arrowBlockObject->tio.object->tio.BLOCK_HEADER) {
        return 0;
    }
    Dwg_Object_BLOCK_HEADER *header =
        arrowBlockObject->tio.object->tio.BLOCK_HEADER;
    NSString *name =
        [JKJDWGReadText(header, "BLOCK_HEADER", "name") uppercaseString];
    if ([name isEqualToString:@"_DOTSMALL"] ||
        [name isEqualToString:@"DOTSMALL"]) {
        return 0.25;
    }
    if ([name isEqualToString:@"_DOT"] ||
        [name isEqualToString:@"DOT"]) {
        return 0.5;
    }
    return 0;
}

static CGMutablePathRef JKJDWGCreateLeaderFilledDot(
    Dwg_Entity_LEADER *leader,
    CGFloat arrowSize,
    CGFloat radiusScale) {
    if (!leader || !leader->points || leader->num_points < 1 ||
        !isfinite(arrowSize) || arrowSize <= DBL_EPSILON ||
        !isfinite(radiusScale) || radiusScale <= DBL_EPSILON) {
        return NULL;
    }
    BITCODE_3DPOINT tip = leader->points[0];
    CGFloat radius = arrowSize * radiusScale;
    if (!isfinite(tip.x) || !isfinite(tip.y) ||
        !isfinite(radius) || radius <= DBL_EPSILON) {
        return NULL;
    }
    CGMutablePathRef dotPath = CGPathCreateMutable();
    CGPathAddEllipseInRect(
        dotPath, NULL,
        CGRectMake(tip.x - radius, tip.y - radius,
                   radius * 2, radius * 2));
    return dotPath;
}

static CGMutablePathRef JKJDWGCreateDefaultLeaderArrow(
    Dwg_Entity_LEADER *leader,
    CGFloat arrowSize) {
    if (!leader || !leader->points || leader->num_points < 2 ||
        !isfinite(arrowSize) || arrowSize <= DBL_EPSILON) {
        return NULL;
    }
    CGPoint tip = CGPointMake(leader->points[0].x,
                              leader->points[0].y);
    CGFloat directionX = leader->points[1].x - tip.x;
    CGFloat directionY = leader->points[1].y - tip.y;
    CGFloat directionLength = hypot(directionX, directionY);
    if (!isfinite(directionLength) || directionLength <= DBL_EPSILON) {
        return NULL;
    }
    directionX /= directionLength;
    directionY /= directionLength;
    CGFloat baseX = tip.x + directionX * arrowSize;
    CGFloat baseY = tip.y + directionY * arrowSize;
    CGFloat halfWidth = arrowSize * 0.35;
    CGFloat normalX = -directionY * halfWidth;
    CGFloat normalY = directionX * halfWidth;

    CGMutablePathRef arrowPath = CGPathCreateMutable();
    CGPathMoveToPoint(arrowPath, NULL, tip.x, tip.y);
    CGPathAddLineToPoint(arrowPath, NULL,
                         baseX + normalX, baseY + normalY);
    CGPathAddLineToPoint(arrowPath, NULL,
                         baseX - normalX, baseY - normalY);
    CGPathCloseSubpath(arrowPath);
    return arrowPath;
}

static BOOL JKJDWGEntityIsDirectlyOwnedByBlock(
    Dwg_Object *entityObject,
    BITCODE_RLL blockHandle) {
    if (!entityObject ||
        entityObject->supertype != DWG_SUPERTYPE_ENTITY ||
        !entityObject->tio.entity ||
        !entityObject->tio.entity->ownerhandle) {
        return NO;
    }
    return entityObject->tio.entity->ownerhandle->absolute_ref == blockHandle;
}

static NSDictionary<NSNumber *, NSIndexSet *> *
JKJDWGBlockEntityIndexesByOwnerHandle(Dwg_Data *dwg,
                                      JKJDWGDrawing *drawing) {
    if (!dwg || !drawing) return @{};
    if (drawing.blockEntityIndexesByOwnerHandle) {
        return drawing.blockEntityIndexesByOwnerHandle;
    }

    // Build this lazily only for malformed ownership lists. One global pass
    // then serves every recovered block reference in the current drawing.
    NSMutableDictionary<NSNumber *, NSIndexSet *> *indexesByOwner =
        [NSMutableDictionary dictionary];
    for (BITCODE_BL index = 0; index < dwg->num_objects; index++) {
        Dwg_Object *object = &dwg->object[index];
        if (object->supertype != DWG_SUPERTYPE_ENTITY ||
            !object->tio.entity || !object->tio.entity->ownerhandle ||
            object->fixedtype == DWG_TYPE_BLOCK ||
            object->fixedtype == DWG_TYPE_ENDBLK) {
            continue;
        }
        NSNumber *ownerHandle =
            @(object->tio.entity->ownerhandle->absolute_ref);
        NSMutableIndexSet *entityIndexes =
            (NSMutableIndexSet *)indexesByOwner[ownerHandle];
        if (!entityIndexes) {
            entityIndexes = [NSMutableIndexSet indexSet];
            indexesByOwner[ownerHandle] = entityIndexes;
        }
        [entityIndexes addIndex:(NSUInteger)index];
    }
    drawing.blockEntityIndexesByOwnerHandle = indexesByOwner;
    return drawing.blockEntityIndexesByOwnerHandle;
}

static void JKJDWGRenderOwnedBlockEntities(
    Dwg_Data *dwg,
    Dwg_Object *blockObject,
    JKJDWGDrawing *drawing,
    CGAffineTransform transform,
    JKJDWGResolvedStyle inheritedStyle,
    CGRect *drawingBounds,
    BOOL *hasDrawingBounds,
    NSMutableSet<NSNumber *> *blockStack,
    NSUInteger childDepth) {
    if (!dwg || !blockObject ||
        blockObject->fixedtype != DWG_TYPE_BLOCK_HEADER ||
        !blockObject->tio.object ||
        !blockObject->tio.object->tio.BLOCK_HEADER) {
        return;
    }

    Dwg_Object_BLOCK_HEADER *header =
        blockObject->tio.object->tio.BLOCK_HEADER;
    BITCODE_RLL blockHandle = blockObject->handle.value;
    if (dwg->header.version < R_2004) {
        Dwg_Object *child = get_first_owned_entity(blockObject);
        while (child) {
            @autoreleasepool {
                JKJDWGRenderBlockObject(
                    dwg, child, drawing, transform, inheritedStyle,
                    drawingBounds, hasDrawingBounds, blockStack, childDepth);
            }
            child = get_next_owned_entity(blockObject, child);
        }
        return;
    }

    if (!header->entities || !header->num_owned) return;

    BOOL requiresRecovery = NO;
    for (BITCODE_BL index = 0; index < header->num_owned; index++) {
        BITCODE_H reference = header->entities[index];
        Dwg_Object *child = reference
            ? dwg_ref_object(dwg, reference)
            : NULL;
        if (!JKJDWGEntityIsDirectlyOwnedByBlock(child, blockHandle)) {
            requiresRecovery = YES;
            break;
        }
    }
    if (!requiresRecovery) {
        Dwg_Object *child = get_first_owned_entity(blockObject);
        while (child) {
            @autoreleasepool {
                JKJDWGRenderBlockObject(
                    dwg, child, drawing, transform, inheritedStyle,
                    drawingBounds, hasDrawingBounds, blockStack, childDepth);
            }
            child = get_next_owned_entity(blockObject, child);
        }
        return;
    }

    NSDictionary<NSNumber *, NSIndexSet *> *indexesByOwner =
        JKJDWGBlockEntityIndexesByOwnerHandle(dwg, drawing);
    NSIndexSet *entityIndexes = indexesByOwner[@(blockHandle)];
    if (entityIndexes.count == 0) return;
    NSUInteger index = entityIndexes.firstIndex;
    while (index != NSNotFound) {
        if (index >= (NSUInteger)dwg->num_objects) break;
        Dwg_Object *child = &dwg->object[index];

        @autoreleasepool {
            JKJDWGRenderBlockObject(
                dwg, child, drawing, transform, inheritedStyle,
                drawingBounds, hasDrawingBounds, blockStack, childDepth);
        }
        index = [entityIndexes indexGreaterThanIndex:index];
    }
}

static void JKJDWGRenderBlockReference(Dwg_Data *dwg,
                                       Dwg_Object *referenceObject,
                                       BITCODE_H blockReference,
                                       BITCODE_3DPOINT insertionPoint,
                                       BITCODE_3DPOINT scale,
                                       BITCODE_BD rotation,
                                       BITCODE_BE extrusion,
                                       JKJDWGDrawing *drawing,
                                       CGAffineTransform parentTransform,
                                       JKJDWGResolvedStyle inheritedStyle,
                                       CGRect *drawingBounds,
                                       BOOL *hasDrawingBounds,
                                       NSMutableSet<NSNumber *> *blockStack,
                                       NSUInteger depth) {
    if (!blockReference || depth > 32) return;
    Dwg_Object *blockObject = dwg_ref_object(dwg, blockReference);
    if (!blockObject || blockObject->fixedtype != DWG_TYPE_BLOCK_HEADER ||
        !blockObject->tio.object || !blockObject->tio.object->tio.BLOCK_HEADER) {
        return;
    }
    NSNumber *blockHandle = @(blockObject->handle.value);
    if ([blockStack containsObject:blockHandle]) return;
    [blockStack addObject:blockHandle];

    Dwg_Object_BLOCK_HEADER *header = blockObject->tio.object->tio.BLOCK_HEADER;
    CGAffineTransform blockTransform = JKJDWGInsertTransform(parentTransform,
                                                             insertionPoint,
                                                             scale,
                                                             rotation,
                                                             extrusion,
                                                             header->base_pt);
    JKJDWGResolvedStyle childStyle = inheritedStyle;
    CGFloat filledDotRadiusScale =
        JKJDWGArrowBlockFilledDotRadiusScale(blockObject);
    if (filledDotRadiusScale > DBL_EPSILON) {
        CGFloat radius = filledDotRadiusScale;
        CGMutablePathRef dotPath = CGPathCreateMutable();
        CGPathAddEllipseInRect(
            dotPath, NULL,
            CGRectMake(header->base_pt.x - radius,
                       header->base_pt.y - radius,
                       radius * 2, radius * 2));
        JKJDWGResolvedStyle dotStyle = childStyle;
        dotStyle.fillPath = YES;
        dotStyle.dashCount = 0;
        dotStyle.lineWidthPoints = 0;
        dotStyle.geometricLineWidth = 0;
        JKJDWGAppendTransformedPath(
            drawing, dotPath, blockTransform, dotStyle,
            drawingBounds, hasDrawingBounds);
        CGPathRelease(dotPath);
        [blockStack removeObject:blockHandle];
        return;
    }
    CGPathRef clipPath = NULL;
    Dwg_Object *spatialFilterObject =
        referenceObject && referenceObject->tio.entity
            ? JKJDWGSpatialFilterObjectForEntity(dwg, referenceObject->tio.entity)
            : NULL;
    if (spatialFilterObject && childStyle.clipCount < 8) {
        Dwg_Object_SPATIAL_FILTER *spatialFilter =
            spatialFilterObject->tio.object->tio.SPATIAL_FILTER;
        clipPath = JKJDWGCreateSpatialClipPath(spatialFilter, blockTransform);
        if (clipPath) {
            childStyle.clipPaths[childStyle.clipCount++] = clipPath;
            childStyle.clipKey =
                childStyle.clipKey * 1099511628211ULL ^
                referenceObject->handle.value ^
                spatialFilterObject->handle.value;
        }
    }
    JKJDWGRenderOwnedBlockEntities(
        dwg, blockObject, drawing, blockTransform, childStyle,
        drawingBounds, hasDrawingBounds, blockStack, depth + 1);
    if (clipPath) CGPathRelease(clipPath);
    [blockStack removeObject:blockHandle];
}

static void JKJDWGRenderBlockObject(Dwg_Data *dwg,
                                    Dwg_Object *object,
                                    JKJDWGDrawing *drawing,
                                    CGAffineTransform transform,
                                    JKJDWGResolvedStyle inheritedStyle,
                                    CGRect *drawingBounds,
                                    BOOL *hasDrawingBounds,
                                    NSMutableSet<NSNumber *> *blockStack,
                                    NSUInteger depth) {
    if (!object || object->supertype != DWG_SUPERTYPE_ENTITY || !object->tio.entity) return;
    JKJDWGResolvedStyle style = JKJDWGStyleForObject(dwg, object, inheritedStyle, depth > 0);
    if (!style.visible) return;
    CGFloat transformScale = MAX(hypot(transform.a, transform.c), DBL_EPSILON);
    if (style.dashCount && fabs(transformScale - 1) > DBL_EPSILON) {
        for (NSUInteger index = 0; index < style.dashCount; index++) {
            style.dashLengths[index] *= transformScale;
        }
    }
    CGMutablePathRef path = CGPathCreateMutable();
    CGPathRef temporaryClipPath = NULL;
    BOOL separatesDrawOrder = NO;
    switch (object->fixedtype) {
        case DWG_TYPE_LINE: {
            Dwg_Entity_LINE *line = object->tio.entity->tio.LINE;
            if (line) {
                CGPathMoveToPoint(path, NULL, line->start.x, line->start.y);
                CGPathAddLineToPoint(path, NULL, line->end.x, line->end.y);
            }
            break;
        }
        case DWG_TYPE_CIRCLE: {
            Dwg_Entity_CIRCLE *circle = object->tio.entity->tio.CIRCLE;
            if (circle && circle->radius > 0) {
                CGMutablePathRef ocsPath = CGPathCreateMutable();
                CGPathAddEllipseInRect(ocsPath, NULL,
                                       CGRectMake(circle->center.x - circle->radius,
                                                  circle->center.y - circle->radius,
                                                  circle->radius * 2,
                                                  circle->radius * 2));
                CGAffineTransform ocsTransform =
                    JKJDWGOCSTransform(circle->extrusion, circle->center.z);
                CGPathAddPath(path, &ocsTransform, ocsPath);
                CGPathRelease(ocsPath);
            }
            break;
        }
        case DWG_TYPE_ARC: {
            Dwg_Entity_ARC *arc = object->tio.entity->tio.ARC;
            if (arc && arc->radius > 0) {
                CGMutablePathRef ocsPath = CGPathCreateMutable();
                CGPathAddArc(ocsPath, NULL, arc->center.x, arc->center.y, arc->radius,
                             arc->start_angle, arc->end_angle, false);
                CGAffineTransform ocsTransform =
                    JKJDWGOCSTransform(arc->extrusion, arc->center.z);
                CGPathAddPath(path, &ocsTransform, ocsPath);
                CGPathRelease(ocsPath);
            }
            break;
        }
        case DWG_TYPE_LWPOLYLINE: {
            Dwg_Entity_LWPOLYLINE *polyline = object->tio.entity->tio.LWPOLYLINE;
            if (polyline && polyline->points && polyline->num_points > 0) {
                CGMutablePathRef ocsPath = CGPathCreateMutable();
                CGPathMoveToPoint(ocsPath, NULL,
                                  polyline->points[0].x, polyline->points[0].y);
                for (BITCODE_BL index = 1; index < polyline->num_points; index++) {
                    double bulge = polyline->bulges && index - 1 < polyline->num_bulges
                        ? polyline->bulges[index - 1] : 0;
                    JKJDWGAddBulgeSegment(ocsPath,
                                          CGPointMake(polyline->points[index - 1].x,
                                                      polyline->points[index - 1].y),
                                          CGPointMake(polyline->points[index].x,
                                                      polyline->points[index].y),
                                          bulge);
                }
                if ((polyline->flag & 512) && polyline->num_points > 1) {
                    BITCODE_BL lastIndex = polyline->num_points - 1;
                    double bulge = polyline->bulges && lastIndex < polyline->num_bulges
                        ? polyline->bulges[lastIndex] : 0;
                    JKJDWGAddBulgeSegment(ocsPath,
                                          CGPointMake(polyline->points[lastIndex].x,
                                                      polyline->points[lastIndex].y),
                                          CGPointMake(polyline->points[0].x,
                                                      polyline->points[0].y),
                                          bulge);
                    CGPathCloseSubpath(ocsPath);
                }
                CGFloat geometricWidth = JKJDWGLWPolylineGeometricWidth(polyline);
                if (geometricWidth > DBL_EPSILON) {
                    style.geometricLineWidth = geometricWidth * transformScale;
                }
                CGAffineTransform ocsTransform =
                    JKJDWGOCSTransform(polyline->extrusion, polyline->elevation);
                CGPathAddPath(path, &ocsTransform, ocsPath);
                CGPathRelease(ocsPath);
            }
            break;
        }
        case DWG_TYPE_POLYLINE_2D: {
            Dwg_Entity_POLYLINE_2D *polyline = object->tio.entity->tio.POLYLINE_2D;
            Dwg_Object *vertexObject = get_first_owned_subentity(object);
            CGMutablePathRef ocsPath = CGPathCreateMutable();
            BOOL hasPoint = NO;
            CGPoint firstPoint = CGPointZero;
            CGPoint previousPoint = CGPointZero;
            double previousBulge = 0;
            while (vertexObject) {
                if (vertexObject->fixedtype == DWG_TYPE_VERTEX_2D &&
                    vertexObject->tio.entity && vertexObject->tio.entity->tio.VERTEX_2D) {
                    Dwg_Entity_VERTEX_2D *vertex = vertexObject->tio.entity->tio.VERTEX_2D;
                    CGPoint point = CGPointMake(vertex->point.x, vertex->point.y);
                    if (!hasPoint) {
                        firstPoint = point;
                        CGPathMoveToPoint(ocsPath, NULL, point.x, point.y);
                    } else {
                        JKJDWGAddBulgeSegment(ocsPath, previousPoint, point, previousBulge);
                    }
                    previousPoint = point;
                    previousBulge = vertex->bulge;
                    hasPoint = YES;
                }
                vertexObject = get_next_owned_subentity(object, vertexObject);
            }
            if (hasPoint && polyline && (polyline->flag & 1)) {
                JKJDWGAddBulgeSegment(ocsPath, previousPoint, firstPoint, previousBulge);
                CGPathCloseSubpath(ocsPath);
            }
            if (hasPoint && polyline) {
                CGFloat geometricWidth =
                    JKJDWGPolyline2DGeometricWidth(object, polyline);
                if (geometricWidth > DBL_EPSILON) {
                    style.geometricLineWidth = geometricWidth * transformScale;
                }
                CGAffineTransform ocsTransform =
                    JKJDWGOCSTransform(polyline->extrusion, polyline->elevation);
                CGPathAddPath(path, &ocsTransform, ocsPath);
            }
            CGPathRelease(ocsPath);
            break;
        }
        case DWG_TYPE_POLYLINE_3D: {
            Dwg_Entity_POLYLINE_3D *polyline = object->tio.entity->tio.POLYLINE_3D;
            Dwg_Object *vertexObject = get_first_owned_subentity(object);
            BOOL hasPoint = NO;
            while (vertexObject) {
                if (vertexObject->fixedtype == DWG_TYPE_VERTEX_3D &&
                    vertexObject->tio.entity && vertexObject->tio.entity->tio.VERTEX_3D) {
                    Dwg_Entity_VERTEX_3D *vertex = vertexObject->tio.entity->tio.VERTEX_3D;
                    if (isfinite(vertex->point.x) && isfinite(vertex->point.y)) {
                        if (hasPoint) {
                            CGPathAddLineToPoint(path, NULL,
                                                 vertex->point.x, vertex->point.y);
                        } else {
                            CGPathMoveToPoint(path, NULL,
                                              vertex->point.x, vertex->point.y);
                            hasPoint = YES;
                        }
                    }
                }
                vertexObject = get_next_owned_subentity(object, vertexObject);
            }
            if (hasPoint && polyline && (polyline->flag & 1)) {
                CGPathCloseSubpath(path);
            }
            break;
        }
        case DWG_TYPE_SOLID:
        case DWG_TYPE_TRACE: {
            Dwg_Entity_SOLID *solid = object->fixedtype == DWG_TYPE_SOLID
                ? object->tio.entity->tio.SOLID
                : (Dwg_Entity_SOLID *)object->tio.entity->tio.TRACE;
            if (solid) {
                CGMutablePathRef ocsPath = CGPathCreateMutable();
                CGPathMoveToPoint(ocsPath, NULL, solid->corner1.x, solid->corner1.y);
                CGPathAddLineToPoint(ocsPath, NULL, solid->corner2.x, solid->corner2.y);
                CGPathAddLineToPoint(ocsPath, NULL, solid->corner4.x, solid->corner4.y);
                CGPathAddLineToPoint(ocsPath, NULL, solid->corner3.x, solid->corner3.y);
                CGPathCloseSubpath(ocsPath);
                CGAffineTransform ocsTransform =
                    JKJDWGOCSTransform(solid->extrusion, solid->elevation);
                CGPathAddPath(path, &ocsTransform, ocsPath);
                CGPathRelease(ocsPath);
            }
            break;
        }
        case DWG_TYPE__3DFACE: {
            Dwg_Entity__3DFACE *face = object->tio.entity->tio._3DFACE;
            if (face) {
                BITCODE_BS hiddenEdges = face->invis_flags;
                BITCODE_3BD corners[] = {
                    face->corner1, face->corner2,
                    face->corner3, face->corner4
                };
                for (NSUInteger edge = 0; edge < 4; edge++) {
                    if (hiddenEdges & (1 << edge)) continue;
                    BITCODE_3BD start = corners[edge];
                    BITCODE_3BD end = corners[(edge + 1) % 4];
                    if (!isfinite(start.x) || !isfinite(start.y) ||
                        !isfinite(end.x) || !isfinite(end.y)) {
                        continue;
                    }
                    CGPathMoveToPoint(path, NULL, start.x, start.y);
                    CGPathAddLineToPoint(path, NULL, end.x, end.y);
                }
            }
            break;
        }
        case DWG_TYPE_POINT: {
            Dwg_Entity_POINT *point = object->tio.entity->tio.POINT;
            NSInteger pointMode = dwg->header_vars.PDMODE;
            if (point && isfinite(point->x) && isfinite(point->y) &&
                pointMode != 1) {
                if (pointMode == 0) {
                    // PDMODE 0 is a viewport-sized dot, while PDMODE 1 is the
                    // mode that suppresses POINT entities. Keep a degenerate
                    // stroke in model space and size its round cap at draw
                    // time so dense point clouds remain visible at any zoom.
                    const CGFloat markerHalfLength = 1e-6;
                    CGPathMoveToPoint(path, NULL,
                                      point->x - markerHalfLength, point->y);
                    CGPathAddLineToPoint(path, NULL,
                                         point->x + markerHalfLength, point->y);
                    style.drawsPointMarkers = YES;
                    style.fillPath = NO;
                    style.dashCount = 0;
                    break;
                }
                CGFloat markerSize = JKJDWGPointMarkerSize(dwg);
                NSInteger baseMode = pointMode & 0x0F;
                if (baseMode == 0) {
                    CGFloat dotRadius = MAX(markerSize * .08, DBL_EPSILON);
                    CGPathAddEllipseInRect(
                        path, NULL,
                        CGRectMake(point->x - dotRadius, point->y - dotRadius,
                                   dotRadius * 2, dotRadius * 2));
                    style.fillPath = YES;
                    style.dashCount = 0;
                } else if (baseMode == 2) {
                    CGPathMoveToPoint(path, NULL,
                                      point->x - markerSize, point->y);
                    CGPathAddLineToPoint(path, NULL,
                                         point->x + markerSize, point->y);
                    CGPathMoveToPoint(path, NULL,
                                      point->x, point->y - markerSize);
                    CGPathAddLineToPoint(path, NULL,
                                         point->x, point->y + markerSize);
                } else if (baseMode == 3) {
                    CGPathMoveToPoint(path, NULL,
                                      point->x - markerSize,
                                      point->y - markerSize);
                    CGPathAddLineToPoint(path, NULL,
                                         point->x + markerSize,
                                         point->y + markerSize);
                    CGPathMoveToPoint(path, NULL,
                                      point->x - markerSize,
                                      point->y + markerSize);
                    CGPathAddLineToPoint(path, NULL,
                                         point->x + markerSize,
                                         point->y - markerSize);
                } else if (baseMode == 4) {
                    CGPathMoveToPoint(path, NULL, point->x, point->y);
                    CGPathAddLineToPoint(path, NULL,
                                         point->x, point->y + markerSize);
                }
                if (pointMode & 32) {
                    CGPathAddEllipseInRect(
                        path, NULL,
                        CGRectMake(point->x - markerSize,
                                   point->y - markerSize,
                                   markerSize * 2, markerSize * 2));
                }
                if (pointMode & 64) {
                    CGPathAddRect(
                        path, NULL,
                        CGRectMake(point->x - markerSize,
                                   point->y - markerSize,
                                   markerSize * 2, markerSize * 2));
                }
            }
            break;
        }
        case DWG_TYPE_ELLIPSE: {
            Dwg_Entity_ELLIPSE *ellipse = object->tio.entity->tio.ELLIPSE;
            if (ellipse) {
                CGAffineTransform ellipseTransform;
                if (JKJDWGEllipseTransform(ellipse, &ellipseTransform)) {
                    CGPathAddArc(path, &ellipseTransform, 0, 0, 1,
                                 ellipse->start_angle, ellipse->end_angle, false);
                }
            }
            break;
        }
        case DWG_TYPE_SPLINE: {
            Dwg_Entity_SPLINE *spline = object->tio.entity->tio.SPLINE;
            BOOL appended = NO;
            if (spline && spline->num_ctrl_pts > spline->degree &&
                spline->ctrl_pts && spline->knots) {
                NSUInteger count = spline->num_ctrl_pts;
                JKJDWGCurveControlPoint *controlPoints =
                    calloc(count, sizeof(JKJDWGCurveControlPoint));
                if (controlPoints) {
                    for (NSUInteger index = 0; index < count; index++) {
                        Dwg_SPLINE_control_point point =
                            spline->ctrl_pts[index];
                        controlPoints[index] =
                            (JKJDWGCurveControlPoint){
                                point.x, point.y,
                                spline->rational || spline->weighted
                                    ? point.w : 1
                            };
                    }
                    appended =
                        JKJDWGAppendNURBSCurve(
                            path, controlPoints, count,
                            spline->knots, spline->num_knots,
                            spline->degree, YES);
                    free(controlPoints);
                }
            }
            if (!appended && spline && spline->num_fit_pts > 1 &&
                spline->fit_pts) {
                NSUInteger count = spline->num_fit_pts;
                CGPoint *fitPoints = calloc(count, sizeof(CGPoint));
                if (fitPoints) {
                    for (NSUInteger index = 0; index < count; index++) {
                        fitPoints[index] =
                            CGPointMake(spline->fit_pts[index].x,
                                        spline->fit_pts[index].y);
                    }
                    appended = JKJDWGAppendFitPointCurve(
                        path, fitPoints, count,
                        spline->closed_b || (spline->splineflags & 4), YES);
                    free(fitPoints);
                }
            }
            if (appended &&
                (spline->closed_b || spline->periodic ||
                 (spline->splineflags & 4))) {
                CGPathCloseSubpath(path);
            }
            break;
        }
        case DWG_TYPE_LEADER: {
            Dwg_Entity_LEADER *leader = object->tio.entity->tio.LEADER;
            if (!leader || !leader->points || leader->num_points < 2) break;

            BOOL hasValidPoints = YES;
            for (BITCODE_BL pointIndex = 0;
                 pointIndex < leader->num_points; pointIndex++) {
                BITCODE_3DPOINT point = leader->points[pointIndex];
                if (!isfinite(point.x) || !isfinite(point.y) ||
                    !isfinite(point.z)) {
                    hasValidPoints = NO;
                    break;
                }
            }
            if (!hasValidPoints) break;

            BOOL appended = NO;
            if (leader->path_type == 1) {
                NSUInteger pointCount = leader->num_points;
                CGPoint *fitPoints = calloc(pointCount, sizeof(CGPoint));
                if (fitPoints) {
                    for (NSUInteger pointIndex = 0;
                         pointIndex < pointCount; pointIndex++) {
                        fitPoints[pointIndex] =
                            CGPointMake(leader->points[pointIndex].x,
                                        leader->points[pointIndex].y);
                    }
                    appended = JKJDWGAppendFitPointCurve(
                        path, fitPoints, pointCount, NO, YES);
                    free(fitPoints);
                }
            }
            if (!appended) {
                CGPathMoveToPoint(path, NULL,
                                  leader->points[0].x,
                                  leader->points[0].y);
                for (BITCODE_BL pointIndex = 1;
                     pointIndex < leader->num_points; pointIndex++) {
                    CGPathAddLineToPoint(path, NULL,
                                         leader->points[pointIndex].x,
                                         leader->points[pointIndex].y);
                }
            }

            if (leader->hookline_on &&
                isfinite(leader->x_direction.x) &&
                isfinite(leader->x_direction.y) &&
                isfinite(leader->box_width) &&
                isfinite(leader->dimgap)) {
                CGFloat directionLength =
                    hypot(leader->x_direction.x,
                          leader->x_direction.y);
                CGFloat hooklineLength =
                    fabs(leader->box_width) + fabs(leader->dimgap);
                if (directionLength > DBL_EPSILON &&
                    hooklineLength > DBL_EPSILON) {
                    CGFloat directionSign =
                        leader->hookline_dir ? 1 : -1;
                    BITCODE_3DPOINT lastPoint =
                        leader->points[leader->num_points - 1];
                    CGFloat hooklineX = lastPoint.x +
                        directionSign * leader->x_direction.x /
                            directionLength * hooklineLength;
                    CGFloat hooklineY = lastPoint.y +
                        directionSign * leader->x_direction.y /
                            directionLength * hooklineLength;
                    if (isfinite(hooklineX) && isfinite(hooklineY)) {
                        CGPathAddLineToPoint(path, NULL,
                                             hooklineX, hooklineY);
                    }
                }
            }

            if (leader->arrowhead_on) {
                Dwg_Object_DIMSTYLE *dimensionStyle =
                    JKJDWGLeaderDimensionStyle(dwg, leader);
                CGFloat arrowSize =
                    JKJDWGLeaderArrowSize(dwg, leader, dimensionStyle);
                BITCODE_H arrowBlock =
                    JKJDWGLeaderArrowBlock(dwg, dimensionStyle);
                Dwg_Object *arrowBlockObject = arrowBlock
                    ? dwg_ref_object(dwg, arrowBlock) : NULL;
                CGFloat filledDotRadiusScale =
                    JKJDWGArrowBlockFilledDotRadiusScale(arrowBlockObject);
                CGMutablePathRef filledArrowPath = NULL;
                if (filledDotRadiusScale > DBL_EPSILON) {
                    filledArrowPath = JKJDWGCreateLeaderFilledDot(
                        leader, arrowSize, filledDotRadiusScale);
                } else if (arrowSize > DBL_EPSILON && arrowBlockObject &&
                    arrowBlockObject->fixedtype == DWG_TYPE_BLOCK_HEADER) {
                    BITCODE_3DPOINT insertionPoint = leader->points[0];
                    BITCODE_3DPOINT scale = {
                        arrowSize, arrowSize, 1
                    };
                    BITCODE_BE extrusion = {0, 0, 1};
                    BITCODE_3DPOINT nextPoint = leader->points[1];
                    CGFloat arrowRotation = atan2(
                        nextPoint.y - insertionPoint.y,
                        nextPoint.x - insertionPoint.x);
                    JKJDWGRenderBlockReference(
                        dwg, object, arrowBlock, insertionPoint, scale,
                        arrowRotation, extrusion, drawing, transform, style,
                        drawingBounds, hasDrawingBounds, blockStack, depth);
                } else {
                    filledArrowPath =
                        JKJDWGCreateDefaultLeaderArrow(leader, arrowSize);
                }
                if (filledArrowPath) {
                    JKJDWGResolvedStyle arrowStyle = style;
                    arrowStyle.fillPath = YES;
                    arrowStyle.dashCount = 0;
                    arrowStyle.lineWidthPoints = 0;
                    arrowStyle.geometricLineWidth = 0;
                    JKJDWGAppendTransformedPath(
                        drawing, filledArrowPath, transform, arrowStyle,
                        drawingBounds, hasDrawingBounds);
                    CGPathRelease(filledArrowPath);
                }
            }
            break;
        }
        case DWG_TYPE_HATCH: {
            Dwg_Entity_HATCH *hatch = object->tio.entity->tio.HATCH;
            BOOL rendersBoundaryFill =
                hatch && (hatch->is_solid_fill || hatch->is_gradient_fill);
            BOOL rendersPattern =
                hatch && !rendersBoundaryFill && hatch->deflines &&
                hatch->num_deflines > 0 && style.clipCount < 8;
            if ((rendersBoundaryFill || rendersPattern) && hatch->paths) {
                for (BITCODE_BL pathIndex = 0; pathIndex < hatch->num_paths; pathIndex++) {
                    Dwg_HATCH_Path *hatchPath = &hatch->paths[pathIndex];
                    if (JKJDWGHatchPathIsDegenerate(hatchPath)) continue;
                    if ((hatchPath->flag & 2) && hatchPath->polyline_paths &&
                        hatchPath->num_segs_or_paths > 0) {
                        BITCODE_2RD firstPoint = hatchPath->polyline_paths[0].point;
                        for (BITCODE_BL pointIndex = 0;
                             pointIndex < hatchPath->num_segs_or_paths;
                             pointIndex++) {
                            BITCODE_2RD point = hatchPath->polyline_paths[pointIndex].point;
                            if (pointIndex == 0) CGPathMoveToPoint(path, NULL, point.x, point.y);
                            else {
                                BITCODE_2RD previousPoint =
                                    hatchPath->polyline_paths[pointIndex - 1].point;
                                JKJDWGAddBulgeSegment(
                                    path,
                                    CGPointMake(previousPoint.x, previousPoint.y),
                                    CGPointMake(point.x, point.y),
                                    hatchPath->polyline_paths[pointIndex - 1].bulge);
                            }
                        }
                        if (hatchPath->closed && hatchPath->num_segs_or_paths > 1) {
                            BITCODE_BL lastIndex = hatchPath->num_segs_or_paths - 1;
                            BITCODE_2RD lastPoint = hatchPath->polyline_paths[lastIndex].point;
                            JKJDWGAddBulgeSegment(
                                path,
                                CGPointMake(lastPoint.x, lastPoint.y),
                                CGPointMake(firstPoint.x, firstPoint.y),
                                hatchPath->polyline_paths[lastIndex].bulge);
                            CGPathCloseSubpath(path);
                        }
                        continue;
                    }
                    if (!hatchPath->segs) continue;
                    uint8_t *ellipseSelections =
                        JKJDWGCreateHatchEllipseSelections(hatchPath);
                    BOOL startedBoundary = NO;
                    for (BITCODE_BL segmentIndex = 0;
                         segmentIndex < hatchPath->num_segs_or_paths;
                         segmentIndex++) {
                        Dwg_HATCH_PathSeg *segment = &hatchPath->segs[segmentIndex];
                        if (segment->curve_type == 1) {
                            if (!startedBoundary) {
                                CGPathMoveToPoint(path, NULL,
                                                  segment->first_endpoint.x,
                                                  segment->first_endpoint.y);
                                startedBoundary = YES;
                            }
                            CGPathAddLineToPoint(path, NULL,
                                                 segment->second_endpoint.x,
                                                 segment->second_endpoint.y);
                        } else if (segment->curve_type == 2 && segment->radius > 0) {
                            // LibreDWG exposes HATCH arc edge angles with a
                            // direction-dependent sign: CCW edges (is_ccw)
                            // keep their true angles, while CW edges are stored
                            // negated (Y-mirrored). Negating unconditionally —
                            // as the previous code did — mirrored the CCW edges
                            // so their start points no longer met the preceding
                            // boundary edge. CoreGraphics then bridged the gap
                            // with a straight segment and swept the arc the wrong
                            // way, distorting solid fills such as the title-block
                            // logo. Negate only the CW edges so every loop stays
                            // continuous.
                            CGFloat hatchAngleSign = segment->is_ccw ? 1.0 : -1.0;
                            CGFloat hatchStartAngle = hatchAngleSign * segment->start_angle;
                            CGFloat hatchEndAngle = hatchAngleSign * segment->end_angle;
                            if (!startedBoundary) {
                                CGPathMoveToPoint(path, NULL,
                                                  segment->center.x +
                                                      cos(hatchStartAngle) * segment->radius,
                                                  segment->center.y +
                                                      sin(hatchStartAngle) * segment->radius);
                                startedBoundary = YES;
                            }
                            CGPathAddArc(path, NULL, segment->center.x, segment->center.y,
                                         segment->radius, hatchStartAngle, hatchEndAngle,
                                         !segment->is_ccw);
                        } else if (segment->curve_type == 3) {
                            CGFloat majorRadius = hypot(segment->endpoint.x, segment->endpoint.y);
                            if (majorRadius > 0 && segment->minor_major_ratio > 0) {
                                CGAffineTransform ellipseTransform =
                                    CGAffineTransformMakeTranslation(segment->center.x,
                                                                     segment->center.y);
                                ellipseTransform = CGAffineTransformRotate(
                                    ellipseTransform,
                                    atan2(segment->endpoint.y, segment->endpoint.x));
                                ellipseTransform = CGAffineTransformScale(
                                    ellipseTransform, majorRadius,
                                    majorRadius * segment->minor_major_ratio);
                                CGFloat hatchStartAngle = segment->start_angle;
                                CGFloat hatchEndAngle = segment->end_angle;
                                if (ellipseSelections) {
                                    if (ellipseSelections[segmentIndex] == 1) {
                                        hatchStartAngle =
                                            -segment->start_angle;
                                        hatchEndAngle =
                                            -segment->end_angle;
                                    }
                                } else if (startedBoundary) {
                                    CGPoint currentPoint =
                                        CGPathGetCurrentPoint(path);
                                    CGPoint rawStartPoint =
                                        CGPointApplyAffineTransform(
                                            CGPointMake(
                                                cos(hatchStartAngle),
                                                sin(hatchStartAngle)),
                                            ellipseTransform);
                                    CGPoint mirroredStartPoint =
                                        CGPointApplyAffineTransform(
                                            CGPointMake(
                                                cos(-hatchStartAngle),
                                                sin(-hatchStartAngle)),
                                            ellipseTransform);
                                    CGFloat rawGap = hypot(
                                        rawStartPoint.x - currentPoint.x,
                                        rawStartPoint.y - currentPoint.y);
                                    CGFloat mirroredGap = hypot(
                                        mirroredStartPoint.x - currentPoint.x,
                                        mirroredStartPoint.y - currentPoint.y);
                                    // Unsupported mixed boundaries retain the
                                    // local continuity fallback.
                                    if (mirroredGap < rawGap) {
                                        hatchStartAngle =
                                            -segment->start_angle;
                                        hatchEndAngle =
                                            -segment->end_angle;
                                    }
                                }
                                if (!startedBoundary) {
                                    CGPoint startPoint = CGPointApplyAffineTransform(
                                        CGPointMake(cos(hatchStartAngle),
                                                    sin(hatchStartAngle)),
                                        ellipseTransform);
                                    CGPathMoveToPoint(path, NULL, startPoint.x, startPoint.y);
                                    startedBoundary = YES;
                                }
                                CGPathAddArc(path, &ellipseTransform, 0, 0, 1,
                                             hatchStartAngle, hatchEndAngle,
                                             !segment->is_ccw);
                            }
                        } else if (segment->curve_type == 4) {
                            BOOL appended = NO;
                            if (segment->num_control_points > segment->degree &&
                                segment->control_points && segment->knots) {
                                NSUInteger count = segment->num_control_points;
                                JKJDWGCurveControlPoint *controlPoints =
                                    calloc(count,
                                           sizeof(JKJDWGCurveControlPoint));
                                if (controlPoints) {
                                    for (NSUInteger pointIndex = 0;
                                         pointIndex < count; pointIndex++) {
                                        Dwg_HATCH_ControlPoint controlPoint =
                                            segment->control_points[pointIndex];
                                        controlPoints[pointIndex] =
                                            (JKJDWGCurveControlPoint){
                                                controlPoint.point.x,
                                                controlPoint.point.y,
                                                segment->is_rational
                                                    ? controlPoint.weight : 1
                                            };
                                    }
                                    appended = JKJDWGAppendNURBSCurve(
                                        path, controlPoints, count,
                                        segment->knots, segment->num_knots,
                                        segment->degree, !startedBoundary);
                                    free(controlPoints);
                                }
                            }
                            if (!appended && segment->num_fitpts > 1 &&
                                segment->fitpts) {
                                NSUInteger count = segment->num_fitpts;
                                CGPoint *fitPoints =
                                    calloc(count, sizeof(CGPoint));
                                if (fitPoints) {
                                    for (NSUInteger pointIndex = 0;
                                         pointIndex < count; pointIndex++) {
                                        fitPoints[pointIndex] =
                                            CGPointMake(
                                                segment->fitpts[pointIndex].x,
                                                segment->fitpts[pointIndex].y);
                                    }
                                    appended = JKJDWGAppendFitPointCurve(
                                        path, fitPoints, count,
                                        segment->is_periodic,
                                        !startedBoundary);
                                    free(fitPoints);
                                }
                            }
                            startedBoundary = startedBoundary || appended;
                        }
                    }
                    free(ellipseSelections);
                    if (startedBoundary && !(hatchPath->flag & 0x20)) {
                        CGPathCloseSubpath(path);
                    }
                }
                if (!CGPathIsEmpty(path)) {
                    CGMutablePathRef boundaryPath = path;
                    CGMutablePathRef sourcePath = boundaryPath;
                    if (rendersPattern) {
                        sourcePath = JKJDWGCreateHatchPatternPath(
                            hatch, boundaryPath);
                    }
                    CGMutablePathRef wcsPath = CGPathCreateMutable();
                    CGAffineTransform ocsTransform =
                        JKJDWGOCSTransform(hatch->extrusion, hatch->elevation);
                    CGPathAddPath(wcsPath, &ocsTransform, sourcePath);
                    if (rendersPattern) {
                        CGMutablePathRef boundaryWCS = CGPathCreateMutable();
                        CGPathAddPath(boundaryWCS, &ocsTransform, boundaryPath);
                        CGMutablePathRef boundaryWorld = CGPathCreateMutable();
                        CGPathAddPath(boundaryWorld, &transform, boundaryWCS);
                        CGPathRelease(boundaryWCS);
                        NSUInteger hatchClipIndex = style.clipCount++;
                        style.clipPaths[hatchClipIndex] = boundaryWorld;
                        // HATCH loops alternate between filled regions and
                        // islands. Non-zero clipping turns inner loops into
                        // filled areas and lets a background pattern leak into
                        // furniture placed over it.
                        if (hatch->style == 0) {
                            style.clipEvenOddMask |= 1UL << hatchClipIndex;
                        }
                        style.clipKey =
                            style.clipKey * 1099511628211ULL ^
                            object->handle.value;
                        CGFloat clipSignature[] = {
                            transform.a, transform.b, transform.c,
                            transform.d, transform.tx, transform.ty
                        };
                        for (NSUInteger signatureIndex = 0;
                             signatureIndex < sizeof(clipSignature) /
                                                  sizeof(clipSignature[0]);
                             signatureIndex++) {
                            int64_t component = (int64_t)llround(
                                clipSignature[signatureIndex] * 1000.0);
                            style.clipKey =
                                (style.clipKey ^ (uint64_t)component) *
                                1099511628211ULL;
                        }
                        style.fillPath = NO;
                        style.dashCount = 0;
                        style.geometricLineWidth = 0;
                        temporaryClipPath = boundaryWorld;
                        CGPathRelease(sourcePath);
                    }
                    CGPathRelease(boundaryPath);
                    path = wcsPath;
                }
            }
            break;
        }
        case DWG_TYPE_WIPEOUT: {
            Dwg_Entity_WIPEOUT *wipeout = object->tio.entity->tio.WIPEOUT;
            if (wipeout && (wipeout->display_props & 1)) {
                BITCODE_BL pointCount =
                    wipeout->clipping && wipeout->clip_verts
                        ? wipeout->num_clip_verts : 0;
                BITCODE_2RD fallbackPoints[4] = {
                    {-0.5, -0.5},
                    {wipeout->image_size.x - 0.5, -0.5},
                    {wipeout->image_size.x - 0.5,
                     wipeout->image_size.y - 0.5},
                    {-0.5, wipeout->image_size.y - 0.5}
                };
                BITCODE_2RD *points =
                    pointCount >= 2 ? wipeout->clip_verts : fallbackPoints;
                if (pointCount < 2) pointCount = 4;
                if (pointCount == 2) {
                    BITCODE_2RD first = points[0];
                    BITCODE_2RD second = points[1];
                    fallbackPoints[0] = first;
                    fallbackPoints[1] = (BITCODE_2RD){second.x, first.y};
                    fallbackPoints[2] = second;
                    fallbackPoints[3] = (BITCODE_2RD){first.x, second.y};
                    points = fallbackPoints;
                    pointCount = 4;
                }
                for (BITCODE_BL index = 0; index < pointCount; index++) {
                    CGFloat imageX = points[index].x + .5;
                    CGFloat imageY = points[index].y + .5;
                    CGFloat x = wipeout->pt0.x +
                        wipeout->uvec.x * imageX + wipeout->vvec.x * imageY;
                    CGFloat y = wipeout->pt0.y +
                        wipeout->uvec.y * imageX + wipeout->vvec.y * imageY;
                    if (index == 0) CGPathMoveToPoint(path, NULL, x, y);
                    else CGPathAddLineToPoint(path, NULL, x, y);
                }
                CGPathCloseSubpath(path);
                style.rgb = 0x13161A;
                style.alpha = 1;
                style.fillPath = YES;
                style.geometricLineWidth = 0;
                style.dashCount = 0;
                style.clipKey ^= object->handle.value;
                [drawing.openChunksByStyle removeAllObjects];
                drawing.styleWindowEntityCount = 0;
                separatesDrawOrder = YES;
            }
            break;
        }
        case DWG_TYPE_TEXT:
        case DWG_TYPE_MTEXT:
        case DWG_TYPE_ATTDEF:
        case DWG_TYPE_ATTRIB: {
            BOOL isMText = object->fixedtype == DWG_TYPE_MTEXT;
            BOOL isAttributeDefinition =
                object->fixedtype == DWG_TYPE_ATTDEF;
            BOOL isAttribute = object->fixedtype == DWG_TYPE_ATTRIB;
            void *entity = isMText
                ? (void *)object->tio.entity->tio.MTEXT
                : isAttributeDefinition
                    ? (void *)object->tio.entity->tio.ATTDEF
                    : isAttribute
                        ? (void *)object->tio.entity->tio.ATTRIB
                        : (void *)object->tio.entity->tio.TEXT;
            NSString *text = JKJDWGReadText(
                entity,
                isMText ? "MTEXT"
                    : isAttributeDefinition ? "ATTDEF"
                    : isAttribute ? "ATTRIB" : "TEXT",
                isMText ? "text"
                    : isAttributeDefinition ? "default_value" : "text_value");
            if (isAttribute &&
                (((Dwg_Entity_ATTRIB *)entity)->flags & 1)) {
                text = nil;
            }
            if (isAttributeDefinition) {
                Dwg_Entity_ATTDEF *definition = entity;
                // Non-constant definitions are templates; their INSERT-owned
                // ATTRIB supplies the actual value. Constant definitions have no
                // corresponding ATTRIB in many drawings and must be rendered here.
                if ((definition->flags & 1) || !(definition->flags & 2)) {
                    text = nil;
                }
            }
            NSString *fontName = isMText ? JKJDWGFontNameFromMText(text) : nil;
            text = JKJDWGDisplayText(text, isMText);
            if (text.length) {
                CGPoint sourcePoint;
                CGFloat sourceHeight;
                CGFloat sourceRotation;
                CGFloat sourceWidthFactor;
                CGFloat sourceObliqueAngle;
                CGFloat sourceAlignmentLength;
                CGFloat horizontalAnchor;
                NSInteger verticalAnchor;
                BOOL verticalText;
                BOOL backwardsText;
                BOOL upsideDownText;
                CGFloat sourceBoxWidth;
                CGFloat sourceBoxHeight;
                BOOL wrapsToBoxWidth;
                BOOL fitsToBoxHeight;
                BOOL clipsToBox;
                sourceAlignmentLength = 0;
                if (isMText) {
                    Dwg_Entity_MTEXT *mtext = entity;
                    Dwg_Object_STYLE *textStyle =
                        JKJDWGTextStyle(dwg, mtext->style);
                    if (!fontName.length) {
                        fontName =
                            JKJDWGFontNameForTextStyle(dwg, mtext->style);
                    }
                    sourcePoint = CGPointMake(mtext->ins_pt.x, mtext->ins_pt.y);
                    sourceHeight =
                        MAX(fabs(mtext->text_height),
                            JKJDWGMinimumTextHeight);
                    sourceRotation = atan2(mtext->x_axis_dir.y, mtext->x_axis_dir.x);
                    sourceWidthFactor =
                        textStyle && isfinite(textStyle->width_factor) &&
                        fabs(textStyle->width_factor) > DBL_EPSILON
                            ? fabs(textStyle->width_factor) : 1;
                    sourceObliqueAngle =
                        textStyle && isfinite(textStyle->oblique_angle)
                            ? textStyle->oblique_angle : 0;
                    backwardsText =
                        textStyle && (textStyle->generation & 2);
                    upsideDownText =
                        textStyle && (textStyle->generation & 4);
                    NSInteger attachment = MIN(MAX((NSInteger)mtext->attachment, 1), 9);
                    horizontalAnchor = (attachment - 1) % 3 / 2.0;
                    verticalAnchor = (attachment - 1) / 3;
                    verticalText = mtext->flow_dir == 3 ||
                        (mtext->flow_dir == 5 &&
                         JKJDWGTextStyleIsVertical(dwg, mtext->style));
                    if (mtext->rect_width > DBL_EPSILON) {
                        // The DWG wrapping width may be narrower than one glyph
                        // when the original SHX font uses different metrics.
                        // Keep wrapping, but give the replacement iOS font at
                        // least the original measured line width and enough
                        // room for one CJK glyph so characters are not clipped.
                        CGFloat measuredLineWidth =
                            MAX(mtext->extents_width, 0);
                        CGFloat fallbackGlyphWidth = sourceHeight * 1.5;
                        CGFloat safeLineWidth = measuredLineWidth;
                        if (measuredLineWidth > mtext->rect_width) {
                            safeLineWidth = MAX(measuredLineWidth,
                                                fallbackGlyphWidth);
                        }
                        sourceBoxWidth = MAX(mtext->rect_width,
                                             safeLineWidth);
                    } else {
                        sourceBoxWidth = mtext->extents_width;
                    }
                    sourceBoxHeight = mtext->rect_height > DBL_EPSILON
                        ? mtext->rect_height : mtext->extents_height;
                    // rect_width is the MTEXT wrapping width. extents_width and
                    // extents_height are measurements made with the original CAD
                    // font, not a clipping rectangle for a replacement iOS font.
                    wrapsToBoxWidth = mtext->rect_width > DBL_EPSILON;
                    fitsToBoxHeight = NO;
                    clipsToBox = NO;
                } else {
                    Dwg_Entity_TEXT syntheticText = {0};
                    Dwg_Entity_TEXT *singleText;
                    if (isAttribute || isAttributeDefinition) {
                        Dwg_Entity_ATTRIB *attribute = isAttribute
                            ? entity : NULL;
                        Dwg_Entity_ATTDEF *definition =
                            isAttributeDefinition ? entity : NULL;
                        syntheticText.elevation = attribute
                            ? attribute->elevation : definition->elevation;
                        syntheticText.ins_pt = attribute
                            ? attribute->ins_pt : definition->ins_pt;
                        syntheticText.alignment_pt = attribute
                            ? attribute->alignment_pt : definition->alignment_pt;
                        syntheticText.extrusion = attribute
                            ? attribute->extrusion : definition->extrusion;
                        syntheticText.oblique_angle = attribute
                            ? attribute->oblique_angle : definition->oblique_angle;
                        syntheticText.rotation = attribute
                            ? attribute->rotation : definition->rotation;
                        syntheticText.height = attribute
                            ? attribute->height : definition->height;
                        syntheticText.width_factor = attribute
                            ? attribute->width_factor : definition->width_factor;
                        syntheticText.generation = attribute
                            ? attribute->generation : definition->generation;
                        syntheticText.horiz_alignment = attribute
                            ? attribute->horiz_alignment : definition->horiz_alignment;
                        syntheticText.vert_alignment = attribute
                            ? attribute->vert_alignment : definition->vert_alignment;
                        syntheticText.style = attribute
                            ? attribute->style : definition->style;
                        singleText = &syntheticText;
                    } else {
                        singleText = entity;
                    }
                    Dwg_Object_STYLE *textStyle =
                        JKJDWGTextStyle(dwg, singleText->style);
                    fontName =
                        JKJDWGFontNameForTextStyle(dwg, singleText->style);
                    BOOL usesAlignmentPoint =
                        singleText->horiz_alignment != 0 || singleText->vert_alignment != 0;
                    sourcePoint = usesAlignmentPoint
                        ? CGPointMake(singleText->alignment_pt.x, singleText->alignment_pt.y)
                        : CGPointMake(singleText->ins_pt.x, singleText->ins_pt.y);
                    CGFloat declaredHeight = fabs(singleText->height);
                    if (declaredHeight <= DBL_EPSILON && textStyle &&
                        isfinite(textStyle->text_size)) {
                        declaredHeight = fabs(textStyle->text_size);
                    }
                    sourceHeight =
                        MAX(declaredHeight, JKJDWGMinimumTextHeight);
                    sourceRotation = singleText->rotation;
                    CGFloat entityWidthFactor =
                        isfinite(singleText->width_factor) &&
                        fabs(singleText->width_factor) > DBL_EPSILON
                            ? fabs(singleText->width_factor) : 1;
                    CGFloat styleWidthFactor =
                        textStyle && isfinite(textStyle->width_factor) &&
                        fabs(textStyle->width_factor) > DBL_EPSILON
                            ? fabs(textStyle->width_factor) : 1;
                    sourceWidthFactor =
                        entityWidthFactor * styleWidthFactor;
                    sourceObliqueAngle =
                        isfinite(singleText->oblique_angle) &&
                        fabs(singleText->oblique_angle) > DBL_EPSILON
                            ? singleText->oblique_angle
                            : (textStyle && isfinite(textStyle->oblique_angle)
                                   ? textStyle->oblique_angle : 0);
                    BITCODE_BS generation =
                        singleText->generation |
                        (textStyle ? textStyle->generation : 0);
                    backwardsText = generation & 2;
                    upsideDownText = generation & 4;
                    verticalText = JKJDWGTextStyleIsVertical(dwg, singleText->style);
                    sourceBoxWidth = 0;
                    sourceBoxHeight = 0;
                    wrapsToBoxWidth = NO;
                    fitsToBoxHeight = NO;
                    clipsToBox = NO;
                    switch (singleText->horiz_alignment) {
                        case 1:
                        case 4:
                            horizontalAnchor = .5;
                            break;
                        case 2:
                            horizontalAnchor = 1;
                            break;
                        default:
                            horizontalAnchor = 0;
                            break;
                    }
                    switch (singleText->vert_alignment) {
                        case 1: verticalAnchor = 2; break;
                        case 2: verticalAnchor = 1; break;
                        case 3: verticalAnchor = 0; break;
                        default: verticalAnchor = -1; break;
                    }
                    if (singleText->horiz_alignment == 3 ||
                        singleText->horiz_alignment == 5) {
                        CGPoint insertionPoint =
                            CGPointMake(singleText->ins_pt.x, singleText->ins_pt.y);
                        CGPoint alignmentPoint =
                            CGPointMake(singleText->alignment_pt.x, singleText->alignment_pt.y);
                        sourcePoint = insertionPoint;
                        sourceRotation = atan2(alignmentPoint.y - insertionPoint.y,
                                               alignmentPoint.x - insertionPoint.x);
                        sourceAlignmentLength =
                            hypot(alignmentPoint.x - insertionPoint.x,
                                  alignmentPoint.y - insertionPoint.y);
                        horizontalAnchor = 0;
                    }
                    CGAffineTransform textOCS =
                        JKJDWGOCSTransform(singleText->extrusion,
                                          singleText->elevation);
                    sourcePoint = CGPointApplyAffineTransform(sourcePoint, textOCS);
                    CGPoint direction = CGPointMake(cos(sourceRotation),
                                                    sin(sourceRotation));
                    CGFloat directionX =
                        textOCS.a * direction.x + textOCS.c * direction.y;
                    CGFloat directionY =
                        textOCS.b * direction.x + textOCS.d * direction.y;
                    if (hypot(directionX, directionY) > DBL_EPSILON) {
                        sourceRotation = atan2(directionY, directionX);
                    }
                }
                JKJDWGTextItem *item = [JKJDWGTextItem new];
                item.text = text;
                item.fontName = fontName;
                item.rgb = style.rgb;
                item.alpha = style.alpha;
                item.point = CGPointApplyAffineTransform(sourcePoint, transform);
                // Adding the INSERT rotation to the text rotation only works
                // for a non-mirrored, uniformly scaled block. Transform the
                // text's own baseline and vertical axes instead so reflected
                // block references keep both their position and direction.
                CGPoint sourceDirection =
                    CGPointMake(cos(sourceRotation), sin(sourceRotation));
                CGPoint sourceVertical =
                    CGPointMake(-sourceDirection.y, sourceDirection.x);
                CGPoint transformedDirection = CGPointMake(
                    transform.a * sourceDirection.x +
                        transform.c * sourceDirection.y,
                    transform.b * sourceDirection.x +
                        transform.d * sourceDirection.y);
                CGPoint transformedVertical = CGPointMake(
                    transform.a * sourceVertical.x +
                        transform.c * sourceVertical.y,
                    transform.b * sourceVertical.x +
                        transform.d * sourceVertical.y);
                CGFloat transformScaleX = MAX(
                    hypot(transformedDirection.x, transformedDirection.y),
                    DBL_EPSILON);
                CGFloat transformScaleY = MAX(
                    hypot(transformedVertical.x, transformedVertical.y),
                    DBL_EPSILON);
                item.height = sourceHeight * transformScaleY;
                item.rotation = atan2(transformedDirection.y,
                                      transformedDirection.x);
                item.widthFactor =
                    sourceWidthFactor * transformScaleX / transformScaleY *
                    (backwardsText ? -1 : 1);
                item.obliqueAngle = sourceObliqueAngle;
                CGFloat transformDeterminant =
                    transform.a * transform.d - transform.b * transform.c;
                BOOL mirrorsTextWithBlock =
                    dwg->header_vars.MIRRTEXT &&
                    transformDeterminant < -DBL_EPSILON;
                item.upsideDown = upsideDownText != mirrorsTextWithBlock;
                item.alignmentLength =
                    sourceAlignmentLength * transformScaleX;
                item.horizontalAnchor = horizontalAnchor;
                item.verticalAnchor = verticalAnchor;
                item.verticalText = verticalText;
                item.boxWidth = sourceBoxWidth * transformScaleX;
                item.boxHeight = sourceBoxHeight * transformScaleY;
                item.wrapsToBoxWidth = wrapsToBoxWidth;
                item.fitsToBoxHeight = fitsToBoxHeight;
                item.clipsToBox = clipsToBox;
                if (style.clipCount) {
                    NSMutableArray<UIBezierPath *> *clippingPaths =
                        [NSMutableArray arrayWithCapacity:style.clipCount];
                    for (NSUInteger index = 0; index < style.clipCount; index++) {
                        if (style.clipPaths[index]) {
                            UIBezierPath *clippingPath =
                                [UIBezierPath bezierPathWithCGPath:
                                    style.clipPaths[index]];
                            clippingPath.usesEvenOddFillRule =
                                (style.clipEvenOddMask & (1UL << index)) != 0;
                            [clippingPaths addObject:clippingPath];
                        }
                    }
                    item.clippingPaths = clippingPaths;
                } else {
                    item.clippingPaths = @[];
                }
                item.maskingPathStartIndex = drawing.maskingPaths.count;
                BOOL canRenderAsGeometry =
                    !isMText && !verticalText && verticalAnchor < 0 &&
                    sourceAlignmentLength <= DBL_EPSILON &&
                    sourceBoxWidth <= DBL_EPSILON &&
                    sourceBoxHeight <= DBL_EPSILON && !clipsToBox;
                if (canRenderAsGeometry) {
                    CGFloat layoutCapHeight = 64;
                    UIFont *probeFont =
                        JKJDWGFont(item.fontName, layoutCapHeight);
                    CGFloat outlineFontSize =
                        layoutCapHeight * layoutCapHeight /
                        MAX(probeFont.capHeight, DBL_EPSILON);
                    UIFont *outlineFont =
                        JKJDWGFont(item.fontName, outlineFontSize);
                    CGFloat lineWidth = 0;
                    CGPathRef glyphPath =
                        JKJDWGCreateTextGlyphPath(item.text, outlineFont,
                                                  &lineWidth);
                    if (glyphPath) {
                        CGFloat glyphScale = item.height /
                            MAX(outlineFont.capHeight, DBL_EPSILON);
                        CGFloat originX =
                            -lineWidth * item.horizontalAnchor;
                        CGAffineTransform textTransform =
                            CGAffineTransformIdentity;
                        textTransform = CGAffineTransformTranslate(
                            textTransform, item.point.x, item.point.y);
                        textTransform = CGAffineTransformRotate(
                            textTransform, item.rotation);
                        textTransform = CGAffineTransformScale(
                            textTransform,
                            item.widthFactor * glyphScale,
                            (item.upsideDown ? -1 : 1) * glyphScale);
                        CGFloat safeObliqueAngle =
                            MAX(MIN(item.obliqueAngle, 1.483529864),
                                -1.483529864);
                        if (fabs(safeObliqueAngle) > DBL_EPSILON) {
                            textTransform = CGAffineTransformConcat(
                                textTransform,
                                CGAffineTransformMake(
                                    1, 0, tan(safeObliqueAngle), 1, 0, 0));
                        }
                        textTransform = CGAffineTransformTranslate(
                            textTransform, originX, 0);
                        JKJDWGResolvedStyle textPathStyle = style;
                        textPathStyle.fillPath = NO;
                        textPathStyle.drawsPointMarkers = NO;
                        textPathStyle.dashCount = 0;
                        textPathStyle.lineWidthPoints = 0;
                        textPathStyle.geometricLineWidth = item.height * 0.05;
                        JKJDWGAppendTransformedPath(
                            drawing, glyphPath, textTransform,
                            textPathStyle, drawingBounds,
                            hasDrawingBounds);
                        CGPathRelease(glyphPath);
                        break;
                    }
                }
                CGFloat estimatedWidth =
                    item.boxWidth > 0 ? item.boxWidth
                    : MAX(item.height,
                          item.height * item.text.length * 1.5 *
                              fabs(item.widthFactor));
                CGFloat estimatedHeight =
                    item.boxHeight > 0 ? item.boxHeight
                    : (verticalText ? item.height * item.text.length : item.height);
                CGFloat radius = hypot(estimatedWidth, estimatedHeight);
                item.drawingBounds = CGRectMake(item.point.x - radius, item.point.y - radius,
                                                radius * 2, radius * 2);
                for (UIBezierPath *clippingPath in item.clippingPaths) {
                    item.drawingBounds =
                        CGRectIntersection(item.drawingBounds,
                                           CGPathGetBoundingBox(clippingPath.CGPath));
                }
                if (CGRectIsNull(item.drawingBounds)) break;
                if (!JKJDWGRectIsSane(drawing, item.drawingBounds)) {
                    drawing.skippedInvalidEntityCount++;
                    break;
                }
                [drawing.texts addObject:item];
                JKJDWGExpandBounds(drawingBounds, hasDrawingBounds,
                                   CGRectGetMinX(item.drawingBounds), CGRectGetMinY(item.drawingBounds));
                JKJDWGExpandBounds(drawingBounds, hasDrawingBounds,
                                   CGRectGetMaxX(item.drawingBounds), CGRectGetMaxY(item.drawingBounds));
                drawing.entityCount++;
            }
            break;
        }
        case DWG_TYPE_DIMENSION_ORDINATE:
        case DWG_TYPE_DIMENSION_LINEAR:
        case DWG_TYPE_DIMENSION_ALIGNED:
        case DWG_TYPE_DIMENSION_ANG3PT:
        case DWG_TYPE_DIMENSION_ANG2LN:
        case DWG_TYPE_DIMENSION_RADIUS:
        case DWG_TYPE_DIMENSION_DIAMETER:
        case DWG_TYPE_ARC_DIMENSION:
        case DWG_TYPE_LARGE_RADIAL_DIMENSION: {
            Dwg_DIMENSION_common *dimension =
                JKJDWGDimensionCommon(object);
            if (!dimension || !dimension->block ||
                !isfinite(dimension->clone_ins_pt.x) ||
                !isfinite(dimension->clone_ins_pt.y) ||
                !isfinite(dimension->elevation) ||
                !isfinite(dimension->ins_scale.x) ||
                !isfinite(dimension->ins_scale.y) ||
                !isfinite(dimension->ins_scale.z) ||
                fabs(dimension->ins_scale.x) <= DBL_EPSILON ||
                fabs(dimension->ins_scale.y) <= DBL_EPSILON ||
                !isfinite(dimension->ins_rotation) ||
                !isfinite(dimension->extrusion.x) ||
                !isfinite(dimension->extrusion.y) ||
                !isfinite(dimension->extrusion.z)) {
                break;
            }
            BITCODE_3DPOINT insertionPoint = {
                dimension->clone_ins_pt.x,
                dimension->clone_ins_pt.y,
                dimension->elevation
            };
            BITCODE_3DPOINT scale = {
                dimension->ins_scale.x,
                dimension->ins_scale.y,
                dimension->ins_scale.z
            };
            JKJDWGRenderBlockReference(
                dwg, object, dimension->block, insertionPoint, scale,
                dimension->ins_rotation, dimension->extrusion,
                drawing, transform, style, drawingBounds, hasDrawingBounds,
                blockStack, depth);
            break;
        }
        case DWG_TYPE_INSERT: {
            Dwg_Entity_INSERT *insert = object->tio.entity->tio.INSERT;
            if (insert) {
                JKJDWGRenderBlockReference(dwg, object,
                                           insert->block_header, insert->ins_pt,
                                           insert->scale, insert->rotation, insert->extrusion,
                                           drawing, transform,
                                           style,
                                           drawingBounds, hasDrawingBounds, blockStack, depth);
                Dwg_Object *attributeObject =
                    get_first_owned_subentity(object);
                while (attributeObject) {
                    if (attributeObject->fixedtype == DWG_TYPE_ATTRIB) {
                        JKJDWGRenderBlockObject(
                            dwg, attributeObject, drawing, transform, style,
                            drawingBounds, hasDrawingBounds, blockStack, depth + 1);
                    }
                    attributeObject =
                        get_next_owned_subentity(object, attributeObject);
                }
            }
            break;
        }
        case DWG_TYPE_MINSERT: {
            Dwg_Entity_MINSERT *insert = object->tio.entity->tio.MINSERT;
            if (insert) {
                NSUInteger rows = MAX((NSUInteger)insert->num_rows, 1);
                NSUInteger columns = MAX((NSUInteger)insert->num_cols, 1);
                for (NSUInteger row = 0; row < rows; row++) {
                    for (NSUInteger column = 0; column < columns; column++) {
                        BITCODE_3DPOINT point = insert->ins_pt;
                        CGFloat offsetX = column * insert->col_spacing;
                        CGFloat offsetY = row * insert->row_spacing;
                        CGFloat cosine = cos(insert->rotation);
                        CGFloat sine = sin(insert->rotation);
                        point.x += offsetX * cosine - offsetY * sine;
                        point.y += offsetX * sine + offsetY * cosine;
                        JKJDWGRenderBlockReference(dwg, object,
                                                   insert->block_header, point,
                                                   insert->scale, insert->rotation,
                                                   insert->extrusion, drawing, transform,
                                                   style,
                                                   drawingBounds, hasDrawingBounds, blockStack, depth);
                        CGAffineTransform attributeTransform =
                            CGAffineTransformTranslate(transform,
                                                       point.x - insert->ins_pt.x,
                                                       point.y - insert->ins_pt.y);
                        Dwg_Object *attributeObject =
                            get_first_owned_subentity(object);
                        while (attributeObject) {
                            if (attributeObject->fixedtype == DWG_TYPE_ATTRIB) {
                                JKJDWGRenderBlockObject(
                                    dwg, attributeObject, drawing,
                                    attributeTransform, style,
                                    drawingBounds, hasDrawingBounds,
                                    blockStack, depth + 1);
                            }
                            attributeObject =
                                get_next_owned_subentity(object, attributeObject);
                        }
                    }
                }
            }
            break;
        }
        default:
            break;
    }
    if (!CGPathIsEmpty(path)) {
        JKJDWGAppendTransformedPath(drawing, path, transform, style,
                                    drawingBounds, hasDrawingBounds);
    }
    if (temporaryClipPath) CGPathRelease(temporaryClipPath);
    if (separatesDrawOrder) {
        CGMutablePathRef worldMaskPath = CGPathCreateMutable();
        CGPathAddPath(worldMaskPath, &transform, path);
        UIBezierPath *maskingPath =
            [UIBezierPath bezierPathWithCGPath:worldMaskPath];
        [drawing.maskingPaths addObject:maskingPath];
        CGPathRelease(worldMaskPath);
        [drawing.openChunksByStyle removeAllObjects];
        drawing.styleWindowEntityCount = 0;
    }
    CGPathRelease(path);
}

static JKJDWGDrawing *JKJDWGDrawingForSpace(Dwg_Data *dwg,
                                             Dwg_Object *spaceObject,
                                             NSString *spaceName) {
    if (!spaceObject || spaceObject->fixedtype != DWG_TYPE_BLOCK_HEADER) return nil;

    JKJDWGDrawing *drawing = [JKJDWGDrawing new];
    drawing.spaceName = spaceName;
    CGRect validationBounds;
    if (JKJDWGModelValidationBounds(dwg, &validationBounds)) {
        drawing.validationBounds = validationBounds;
        drawing.hasValidationBounds = YES;
    }
    NSUInteger endCap = dwg->header_vars.ENDCAPS;
    if (endCap > 3) endCap = (endCap & 0x60) >> 5;
    drawing.lineCap = endCap == 2 ? kCGLineCapSquare
                    : endCap == 3 ? kCGLineCapRound
                                  : kCGLineCapButt;
    NSUInteger joinStyle = dwg->header_vars.JOINSTYLE;
    if (joinStyle > 3) joinStyle = (joinStyle & 0x180) >> 7;
    drawing.lineJoin = joinStyle == 2 ? kCGLineJoinBevel
                     : joinStyle == 3 ? kCGLineJoinRound
                                      : kCGLineJoinMiter;
    CGRect bounds = CGRectZero;
    BOOL hasBounds = NO;
    NSMutableSet<NSNumber *> *blockStack = [NSMutableSet set];
    Dwg_Object *entity = get_first_owned_entity(spaceObject);
    while (entity) {
        @autoreleasepool {
            JKJDWGRenderBlockObject(dwg, entity, drawing, CGAffineTransformIdentity,
                                    JKJDWGDefaultStyle(),
                                    &bounds, &hasBounds, blockStack, 0);
        }
        entity = get_next_owned_entity(spaceObject, entity);
    }
    drawing.blockEntityIndexesByOwnerHandle = nil;
    if (drawing.skippedInvalidEntityCount) {
        NSLog(@"[DWG] 已跳过 %lu 个坐标异常的图元",
              (unsigned long)drawing.skippedInvalidEntityCount);
    }

    if (!hasBounds || drawing.entityCount == 0) return nil;
    if (bounds.size.width < DBL_EPSILON) bounds.size.width = 1;
    if (bounds.size.height < DBL_EPSILON) bounds.size.height = 1;
    drawing.drawingBounds = CGRectInset(bounds,
                                        -MAX(bounds.size.width * 0.02, 1),
                                        -MAX(bounds.size.height * 0.02, 1));
    drawing.currentChunk = nil;
    drawing.openChunksByStyle = nil;
    return drawing;
}

+ (NSArray<JKJDWGDrawing *> *)drawingsWithFile:(NSString *)filePath error:(NSError **)error {
    NSString *normalizedPath = filePath;
    if ([filePath hasPrefix:@"file://"]) {
        normalizedPath = [NSURL URLWithString:filePath].path ?: filePath;
    }
    if (![[NSFileManager defaultManager] fileExistsAtPath:normalizedPath]) {
        if (error) {
            *error = [NSError errorWithDomain:JKJDWGErrorDomain code:DWG_ERR_IOERROR
                                     userInfo:@{NSLocalizedDescriptionKey: @"DWG 本地文件不存在"}];
        }
        return nil;
    }

    Dwg_Data dwg = {0};
    int readError = dwg_read_file(normalizedPath.fileSystemRepresentation, &dwg);
    if (readError >= DWG_ERR_CRITICAL) {
        if (error) {
            NSString *message = [NSString stringWithFormat:@"DWG 解析失败（LibreDWG 错误码 %d）",
                                                           readError];
            *error = [NSError errorWithDomain:JKJDWGErrorDomain code:readError
                                     userInfo:@{NSLocalizedDescriptionKey: message}];
        }
        dwg_free(&dwg);
        return nil;
    }
    if (readError != 0) {
        NSLog(@"[DWG] LibreDWG 解析警告码 %d；将过滤异常坐标后继续展示",
              readError);
    }

    Dwg_Object *modelSpace = dwg_model_space_object(&dwg);
    JKJDWGDrawing *modelDrawing = JKJDWGDrawingForSpace(&dwg, modelSpace, @"模型");
    dwg_free(&dwg);

    if (!modelDrawing && error) {
        *error = [NSError errorWithDomain:JKJDWGErrorDomain code:DWG_ERR_INVALIDDWG
                                 userInfo:@{NSLocalizedDescriptionKey:
                                                @"模型空间中没有可展示的二维图元"}];
    }
    return modelDrawing ? @[modelDrawing] : nil;
}

+ (instancetype)drawingWithFile:(NSString *)filePath error:(NSError **)error {
    Dwg_Data dwg = {0};
    int readError = dwg_read_file(filePath.fileSystemRepresentation, &dwg);
    if (readError >= DWG_ERR_CRITICAL) {
        if (error) {
            *error = [NSError errorWithDomain:JKJDWGErrorDomain code:readError
                                     userInfo:@{NSLocalizedDescriptionKey: @"DWG 文件解析失败或版本暂不受支持"}];
        }
        dwg_free(&dwg);
        return nil;
    }

    JKJDWGDrawing *drawing = [JKJDWGDrawing new];
    CGRect bounds = CGRectZero;
    BOOL hasBounds = NO;
    Dwg_Object *modelSpace = dwg_model_space_object(&dwg);
    NSMutableSet<NSNumber *> *blockStack = [NSMutableSet set];
    for (BITCODE_BL index = 0; index < dwg.num_objects; index++) {
        Dwg_Object *object = &dwg.object[index];
        if (object->supertype != DWG_SUPERTYPE_ENTITY || !object->tio.entity) continue;
        if (modelSpace && object->tio.entity->ownerhandle) {
            Dwg_Object *owner = dwg_ref_object(&dwg, object->tio.entity->ownerhandle);
            if (owner && owner != modelSpace) continue;
        }

        switch (object->fixedtype) {
            case DWG_TYPE_INSERT:
            case DWG_TYPE_MINSERT:
            case DWG_TYPE_POLYLINE_2D:
            case DWG_TYPE_POLYLINE_3D:
            case DWG_TYPE_HATCH:
            case DWG_TYPE_SOLID:
            case DWG_TYPE_TRACE:
            case DWG_TYPE__3DFACE:
                JKJDWGRenderBlockObject(&dwg, object, drawing, CGAffineTransformIdentity,
                                        JKJDWGDefaultStyle(),
                                        &bounds, &hasBounds, blockStack, 0);
                break;
            case DWG_TYPE_LINE: {
                Dwg_Entity_LINE *line = object->tio.entity->tio.LINE;
                if (!line) break;
                JKJDWGPathChunk *chunk = [drawing chunkForNextEntity];
                CGPathMoveToPoint(chunk.path, NULL, line->start.x, line->start.y);
                CGPathAddLineToPoint(chunk.path, NULL, line->end.x, line->end.y);
                JKJDWGExpandBounds(&bounds, &hasBounds, line->start.x, line->start.y);
                JKJDWGExpandBounds(&bounds, &hasBounds, line->end.x, line->end.y);
                JKJDWGExpandChunkBounds(chunk, line->start.x, line->start.y);
                JKJDWGExpandChunkBounds(chunk, line->end.x, line->end.y);
                drawing.entityCount++;
                break;
            }
            case DWG_TYPE_CIRCLE: {
                Dwg_Entity_CIRCLE *circle = object->tio.entity->tio.CIRCLE;
                if (!circle || circle->radius <= 0) break;
                CGRect circleRect = CGRectMake(circle->center.x - circle->radius,
                                               circle->center.y - circle->radius,
                                               circle->radius * 2, circle->radius * 2);
                JKJDWGPathChunk *chunk = [drawing chunkForNextEntity];
                CGPathAddEllipseInRect(chunk.path, NULL, circleRect);
                JKJDWGExpandBounds(&bounds, &hasBounds, CGRectGetMinX(circleRect), CGRectGetMinY(circleRect));
                JKJDWGExpandBounds(&bounds, &hasBounds, CGRectGetMaxX(circleRect), CGRectGetMaxY(circleRect));
                JKJDWGExpandChunkBounds(chunk, CGRectGetMinX(circleRect), CGRectGetMinY(circleRect));
                JKJDWGExpandChunkBounds(chunk, CGRectGetMaxX(circleRect), CGRectGetMaxY(circleRect));
                drawing.entityCount++;
                break;
            }
            case DWG_TYPE_ARC: {
                Dwg_Entity_ARC *arc = object->tio.entity->tio.ARC;
                if (!arc || arc->radius <= 0) break;
                JKJDWGPathChunk *chunk = [drawing chunkForNextEntity];
                CGPathAddArc(chunk.path, NULL, arc->center.x, arc->center.y, arc->radius,
                             arc->start_angle, arc->end_angle, false);
                JKJDWGExpandBounds(&bounds, &hasBounds, arc->center.x - arc->radius, arc->center.y - arc->radius);
                JKJDWGExpandBounds(&bounds, &hasBounds, arc->center.x + arc->radius, arc->center.y + arc->radius);
                JKJDWGExpandChunkBounds(chunk, arc->center.x - arc->radius, arc->center.y - arc->radius);
                JKJDWGExpandChunkBounds(chunk, arc->center.x + arc->radius, arc->center.y + arc->radius);
                drawing.entityCount++;
                break;
            }
            case DWG_TYPE_LWPOLYLINE: {
                Dwg_Entity_LWPOLYLINE *polyline = object->tio.entity->tio.LWPOLYLINE;
                if (!polyline || !polyline->points || polyline->num_points == 0) break;
                JKJDWGPathChunk *chunk = [drawing chunkForNextEntity];
                CGPathMoveToPoint(chunk.path, NULL, polyline->points[0].x, polyline->points[0].y);
                for (BITCODE_BL pointIndex = 0; pointIndex < polyline->num_points; pointIndex++) {
                    if (pointIndex > 0) {
                        double bulge = polyline->bulges && pointIndex - 1 < polyline->num_bulges
                            ? polyline->bulges[pointIndex - 1] : 0;
                        JKJDWGAddBulgeSegment(
                            chunk.path,
                            CGPointMake(polyline->points[pointIndex - 1].x,
                                        polyline->points[pointIndex - 1].y),
                            CGPointMake(polyline->points[pointIndex].x,
                                        polyline->points[pointIndex].y),
                            bulge);
                    }
                    JKJDWGExpandBounds(&bounds, &hasBounds,
                                       polyline->points[pointIndex].x,
                                       polyline->points[pointIndex].y);
                    JKJDWGExpandChunkBounds(chunk,
                                            polyline->points[pointIndex].x,
                                            polyline->points[pointIndex].y);
                }
                if ((polyline->flag & 512) && polyline->num_points > 1) {
                    BITCODE_BL lastIndex = polyline->num_points - 1;
                    double bulge = polyline->bulges && lastIndex < polyline->num_bulges
                        ? polyline->bulges[lastIndex] : 0;
                    JKJDWGAddBulgeSegment(
                        chunk.path,
                        CGPointMake(polyline->points[lastIndex].x,
                                    polyline->points[lastIndex].y),
                        CGPointMake(polyline->points[0].x,
                                    polyline->points[0].y),
                        bulge);
                    CGPathCloseSubpath(chunk.path);
                }
                drawing.entityCount++;
                break;
            }
            case DWG_TYPE_ELLIPSE: {
                Dwg_Entity_ELLIPSE *ellipse = object->tio.entity->tio.ELLIPSE;
                if (!ellipse) break;
                CGAffineTransform transform;
                if (!JKJDWGEllipseTransform(ellipse, &transform)) break;
                CGMutablePathRef ellipsePath = CGPathCreateMutable();
                CGPathAddArc(ellipsePath, NULL, 0, 0, 1, ellipse->start_angle, ellipse->end_angle, false);
                CGPathRef transformedPath =
                    CGPathCreateCopyByTransformingPath(ellipsePath, &transform);
                CGRect ellipseBounds = CGPathGetPathBoundingBox(transformedPath);
                JKJDWGPathChunk *chunk = [drawing chunkForNextEntity];
                CGPathAddPath(chunk.path, NULL, transformedPath);
                CGPathRelease(transformedPath);
                CGPathRelease(ellipsePath);
                JKJDWGExpandBounds(&bounds, &hasBounds,
                                   CGRectGetMinX(ellipseBounds),
                                   CGRectGetMinY(ellipseBounds));
                JKJDWGExpandBounds(&bounds, &hasBounds,
                                   CGRectGetMaxX(ellipseBounds),
                                   CGRectGetMaxY(ellipseBounds));
                JKJDWGExpandChunkBounds(chunk,
                                        CGRectGetMinX(ellipseBounds),
                                        CGRectGetMinY(ellipseBounds));
                JKJDWGExpandChunkBounds(chunk,
                                        CGRectGetMaxX(ellipseBounds),
                                        CGRectGetMaxY(ellipseBounds));
                drawing.entityCount++;
                break;
            }
            case DWG_TYPE_POINT: {
                Dwg_Entity_POINT *point = object->tio.entity->tio.POINT;
                if (!point) break;
                JKJDWGPathChunk *chunk = [drawing chunkForNextEntity];
                CGPathMoveToPoint(chunk.path, NULL, point->x - 1, point->y);
                CGPathAddLineToPoint(chunk.path, NULL, point->x + 1, point->y);
                CGPathMoveToPoint(chunk.path, NULL, point->x, point->y - 1);
                CGPathAddLineToPoint(chunk.path, NULL, point->x, point->y + 1);
                JKJDWGExpandBounds(&bounds, &hasBounds, point->x, point->y);
                JKJDWGExpandChunkBounds(chunk, point->x - 1, point->y - 1);
                JKJDWGExpandChunkBounds(chunk, point->x + 1, point->y + 1);
                drawing.entityCount++;
                break;
            }
            case DWG_TYPE_SPLINE: {
                Dwg_Entity_SPLINE *spline = object->tio.entity->tio.SPLINE;
                if (!spline) break;
                if (spline->num_fit_pts > 1 && spline->fit_pts) {
                    JKJDWGPathChunk *chunk = [drawing chunkForNextEntity];
                    for (BITCODE_BS pointIndex = 0; pointIndex < spline->num_fit_pts; pointIndex++) {
                        if (pointIndex == 0) CGPathMoveToPoint(chunk.path, NULL, spline->fit_pts[pointIndex].x, spline->fit_pts[pointIndex].y);
                        else CGPathAddLineToPoint(chunk.path, NULL, spline->fit_pts[pointIndex].x, spline->fit_pts[pointIndex].y);
                        JKJDWGExpandBounds(&bounds, &hasBounds, spline->fit_pts[pointIndex].x, spline->fit_pts[pointIndex].y);
                        JKJDWGExpandChunkBounds(chunk, spline->fit_pts[pointIndex].x, spline->fit_pts[pointIndex].y);
                    }
                    drawing.entityCount++;
                } else if (spline->num_ctrl_pts > 1 && spline->ctrl_pts) {
                    JKJDWGPathChunk *chunk = [drawing chunkForNextEntity];
                    for (BITCODE_BL pointIndex = 0; pointIndex < spline->num_ctrl_pts; pointIndex++) {
                        if (pointIndex == 0) CGPathMoveToPoint(chunk.path, NULL, spline->ctrl_pts[pointIndex].x, spline->ctrl_pts[pointIndex].y);
                        else CGPathAddLineToPoint(chunk.path, NULL, spline->ctrl_pts[pointIndex].x, spline->ctrl_pts[pointIndex].y);
                        JKJDWGExpandBounds(&bounds, &hasBounds, spline->ctrl_pts[pointIndex].x, spline->ctrl_pts[pointIndex].y);
                        JKJDWGExpandChunkBounds(chunk, spline->ctrl_pts[pointIndex].x, spline->ctrl_pts[pointIndex].y);
                    }
                    drawing.entityCount++;
                }
                break;
            }
            case DWG_TYPE_TEXT:
            case DWG_TYPE_MTEXT: {
                BOOL isMText = object->fixedtype == DWG_TYPE_MTEXT;
                void *entity = isMText ? (void *)object->tio.entity->tio.MTEXT : (void *)object->tio.entity->tio.TEXT;
                NSString *text = JKJDWGReadText(entity, isMText ? "MTEXT" : "TEXT",
                                                isMText ? "text" : "text_value");
                text = JKJDWGDisplayText(text, isMText);
                if (!text.length) break;
                JKJDWGTextItem *item = [JKJDWGTextItem new];
                item.text = text;
                if (isMText) {
                    Dwg_Entity_MTEXT *mtext = entity;
                    item.point = CGPointMake(mtext->ins_pt.x, mtext->ins_pt.y);
                    item.height =
                        MAX(fabs(mtext->text_height),
                            JKJDWGMinimumTextHeight);
                    item.rotation = atan2(mtext->x_axis_dir.y, mtext->x_axis_dir.x);
                } else {
                    Dwg_Entity_TEXT *singleText = entity;
                    item.point = CGPointMake(singleText->ins_pt.x, singleText->ins_pt.y);
                    item.height =
                        MAX(fabs(singleText->height),
                            JKJDWGMinimumTextHeight);
                    item.rotation = singleText->rotation;
                }
                CGFloat estimatedWidth = MAX(item.height, item.height * item.text.length * 1.5);
                CGFloat textRadius = hypot(estimatedWidth, item.height);
                item.drawingBounds = CGRectMake(item.point.x - textRadius,
                                                item.point.y - textRadius,
                                                textRadius * 2,
                                                textRadius * 2);
                [drawing.texts addObject:item];
                JKJDWGExpandBounds(&bounds, &hasBounds,
                                   CGRectGetMinX(item.drawingBounds),
                                   CGRectGetMinY(item.drawingBounds));
                JKJDWGExpandBounds(&bounds, &hasBounds,
                                   CGRectGetMaxX(item.drawingBounds),
                                   CGRectGetMaxY(item.drawingBounds));
                drawing.entityCount++;
                break;
            }
            default:
                break;
        }
    }
    drawing.blockEntityIndexesByOwnerHandle = nil;
    dwg_free(&dwg);

    if (!hasBounds || drawing.entityCount == 0) {
        if (error) {
            *error = [NSError errorWithDomain:JKJDWGErrorDomain code:DWG_ERR_INVALIDDWG
                                     userInfo:@{NSLocalizedDescriptionKey: @"DWG 中没有可展示的二维图元"}];
        }
        return nil;
    }
    if (bounds.size.width < DBL_EPSILON) bounds.size.width = 1;
    if (bounds.size.height < DBL_EPSILON) bounds.size.height = 1;
    drawing.drawingBounds = CGRectInset(bounds, -bounds.size.width * 0.02, -bounds.size.height * 0.02);
    drawing.currentChunk = nil;
    drawing.openChunksByStyle = nil;
    return drawing;
}

@end

@interface JKJDWGTiledLayer : CATiledLayer
@end

@implementation JKJDWGTiledLayer
+ (CFTimeInterval)fadeDuration {
    return 0;
}
@end

@interface JKJDWGTiledCanvasView : UIView
@property (nonatomic, strong) JKJDWGDrawing *drawing;
@property (atomic) CGFloat viewportZoomScale;
@end

@implementation JKJDWGTiledCanvasView

+ (Class)layerClass {
    return JKJDWGTiledLayer.class;
}

- (instancetype)init {
    self = [super init];
    if (self) {
        _viewportZoomScale = 1;
        self.opaque = YES;
        self.backgroundColor = [UIColor colorWithRed:0.075 green:0.086 blue:0.102 alpha:1];
        CATiledLayer *tiledLayer = (CATiledLayer *)self.layer;
        CGFloat screenScale = UIScreen.mainScreen.scale;
        tiledLayer.tileSize = CGSizeMake(256, 256);
        tiledLayer.levelsOfDetail = 21;
        tiledLayer.levelsOfDetailBias = 16;
        tiledLayer.contentsScale = screenScale;
    }
    return self;
}

- (void)setDrawing:(JKJDWGDrawing *)drawing {
    _drawing = drawing;
    [self.layer setNeedsDisplay];
}

- (void)drawRect:(CGRect)rect {
    JKJDWGDrawing *drawing = self.drawing;
    if (!drawing) return;

    CGContextRef context = UIGraphicsGetCurrentContext();
    CGRect drawingBounds = drawing.drawingBounds;
    CGFloat scale = MIN((CGRectGetWidth(self.bounds) - 32) / CGRectGetWidth(drawingBounds),
                        (CGRectGetHeight(self.bounds) - 32) / CGRectGetHeight(drawingBounds));
    CGFloat contentWidth = CGRectGetWidth(drawingBounds) * scale;
    CGFloat contentHeight = CGRectGetHeight(drawingBounds) * scale;
    CGFloat offsetX = (CGRectGetWidth(self.bounds) - contentWidth) * 0.5;
    CGFloat offsetY = (CGRectGetHeight(self.bounds) - contentHeight) * 0.5;
    CGFloat effectiveScale =
        scale * MAX(self.viewportZoomScale, JKJDWGMinimumZoomScale);
    CGRect visibleRect = CGRectInset(rect, -4, -4);

    CGContextSetFillColorWithColor(context, self.backgroundColor.CGColor);
    CGContextFillRect(context, rect);

    CGContextSaveGState(context);
    CGContextTranslateCTM(context, offsetX - CGRectGetMinX(drawingBounds) * scale,
                          offsetY + contentHeight + CGRectGetMinY(drawingBounds) * scale);
    CGContextScaleCTM(context, scale, -scale);
    CGContextSetLineJoin(context, drawing.lineJoin);
    CGContextSetLineCap(context, drawing.lineCap);
    for (JKJDWGPathChunk *chunk in drawing.pathChunks) {
        CGRect chunkBounds = chunk.drawingBounds;
        CGRect chunkViewBounds = CGRectMake(
            offsetX + (CGRectGetMinX(chunkBounds) - CGRectGetMinX(drawingBounds)) * scale,
            offsetY + contentHeight - (CGRectGetMaxY(chunkBounds) - CGRectGetMinY(drawingBounds)) * scale,
            MAX(CGRectGetWidth(chunkBounds) * scale, 1),
            MAX(CGRectGetHeight(chunkBounds) * scale, 1)
        );
        if (CGRectIntersectsRect(visibleRect, CGRectInset(chunkViewBounds, -2, -2))) {
            CGContextSaveGState(context);
            for (UIBezierPath *clippingPath in chunk.clippingPaths) {
                CGContextAddPath(context, clippingPath.CGPath);
                if (clippingPath.usesEvenOddFillRule) {
                    CGContextEOClip(context);
                } else {
                    CGContextClip(context);
                }
            }
            CGFloat red = ((chunk.rgb >> 16) & 0xFF) / 255.0;
            CGFloat green = ((chunk.rgb >> 8) & 0xFF) / 255.0;
            CGFloat blue = (chunk.rgb & 0xFF) / 255.0;
            UIColor *color = [UIColor colorWithRed:red green:green blue:blue
                                             alpha:MAX(chunk.alpha, 0.05)];
            CGContextSetStrokeColorWithColor(context, color.CGColor);
            CGContextSetFillColorWithColor(context, color.CGColor);
            CGFloat lineWidth = chunk.geometricLineWidth > DBL_EPSILON
                ? chunk.geometricLineWidth
                : MAX(chunk.lineWidthPoints / effectiveScale,
                      0.45 / effectiveScale);
            if (chunk.drawsPointMarkers) {
                CGContextSetLineCap(context, kCGLineCapRound);
                lineWidth = MAX(lineWidth, 1.0 / effectiveScale);
            }
            CGContextSetLineWidth(context, lineWidth);
            if (chunk.dashPattern.count) {
                CGFloat lengths[32] = {0};
                NSUInteger count = MIN(chunk.dashPattern.count, 32);
                for (NSUInteger index = 0; index < count; index++) {
                    lengths[index] = MAX(chunk.dashPattern[index].doubleValue, 0.01 / scale);
                }
                CGContextSetLineDash(context, 0, lengths, count);
            } else {
                CGContextSetLineDash(context, 0, NULL, 0);
            }
            CGContextAddPath(context, chunk.path);
            CGContextDrawPath(context, chunk.fillPath
                                      ? kCGPathEOFill
                                      : kCGPathStroke);
            CGContextRestoreGState(context);
        }
    }
    CGContextRestoreGState(context);

    visibleRect = CGRectInset(rect, -8, -8);
    for (JKJDWGTextItem *item in drawing.texts) {
        CGRect itemBounds = item.drawingBounds;
        CGRect itemViewBounds = CGRectMake(
            offsetX + (CGRectGetMinX(itemBounds) - CGRectGetMinX(drawingBounds)) * scale,
            offsetY + contentHeight - (CGRectGetMaxY(itemBounds) - CGRectGetMinY(drawingBounds)) * scale,
            MAX(CGRectGetWidth(itemBounds) * scale, 1),
            MAX(CGRectGetHeight(itemBounds) * scale, 1)
        );
        if (!CGRectIntersectsRect(visibleRect, itemViewBounds)) continue;
        CGPoint point = CGPointMake(offsetX + (item.point.x - CGRectGetMinX(drawingBounds)) * scale,
                                    offsetY + contentHeight - (item.point.y - CGRectGetMinY(drawingBounds)) * scale);
        CGFloat targetCapHeight =
            MIN(MAX(item.height * scale, JKJDWGMinimumRenderedTextSize), 1024);
        CGFloat red = ((item.rgb >> 16) & 0xFF) / 255.0;
        CGFloat green = ((item.rgb >> 8) & 0xFF) / 255.0;
        CGFloat blue = (item.rgb & 0xFF) / 255.0;
        UIFont *probeFont = JKJDWGFont(item.fontName, targetCapHeight);
        CGFloat fontSize = targetCapHeight * targetCapHeight /
            MAX(probeFont.capHeight, DBL_EPSILON);
        UIFont *font = JKJDWGFont(item.fontName, fontSize);
        NSDictionary *attributes = @{
            NSForegroundColorAttributeName:
                [UIColor colorWithRed:red green:green blue:blue alpha:MAX(item.alpha, 0.05)],
            NSFontAttributeName: font
        };
        CGFloat localBoxWidth = item.boxWidth > 0
            ? item.boxWidth * scale / MAX(fabs(item.widthFactor), 0.01) : 0;
        CGFloat localBoxHeight = item.boxHeight > 0 ? item.boxHeight * scale : 0;
        CGSize constraint = CGSizeMake(item.wrapsToBoxWidth && localBoxWidth > 0
                                           ? localBoxWidth : CGFLOAT_MAX,
                                       CGFLOAT_MAX);
        CGSize textSize = [item.text boundingRectWithSize:constraint
                                                 options:NSStringDrawingUsesLineFragmentOrigin |
                                                         NSStringDrawingUsesFontLeading
                                              attributes:attributes
                                                 context:nil].size;
        if (!item.verticalText && item.fitsToBoxHeight && localBoxHeight > 0 &&
            textSize.height > localBoxHeight && textSize.height > DBL_EPSILON) {
            CGFloat fitScale = MIN(localBoxHeight / textSize.height, 1) * .98;
            fontSize =
                MAX(fontSize * fitScale, JKJDWGMinimumRenderedTextSize);
            font = JKJDWGFont(item.fontName, fontSize);
            attributes = @{
                NSForegroundColorAttributeName:
                    [UIColor colorWithRed:red green:green blue:blue alpha:MAX(item.alpha, 0.05)],
                NSFontAttributeName: font
            };
            textSize = [item.text boundingRectWithSize:constraint
                                               options:NSStringDrawingUsesLineFragmentOrigin |
                                                       NSStringDrawingUsesFontLeading
                                            attributes:attributes
                                               context:nil].size;
        }
        CGFloat measuredWidth =
            MAX(textSize.width, JKJDWGMinimumRenderedTextSize);
        CGFloat measuredHeight =
            MAX(textSize.height, JKJDWGMinimumRenderedTextSize);
        CGFloat layoutWidth =
            item.wrapsToBoxWidth && localBoxWidth > 0
                ? localBoxWidth
                : MAX(localBoxWidth, measuredWidth);
        CGFloat layoutHeight =
            (item.fitsToBoxHeight || item.clipsToBox) && localBoxHeight > 0
                ? localBoxHeight
                : MAX(localBoxHeight, measuredHeight);
        CGFloat originX = -layoutWidth * item.horizontalAnchor;
        CGFloat originY;
        if (item.verticalAnchor < 0) originY = -font.ascender;
        else if (item.verticalAnchor == 0) originY = 0;
        else if (item.verticalAnchor == 1) originY = -layoutHeight * .5;
        else originY = -layoutHeight;

        CGFloat drawWidthFactor =
            fabs(item.widthFactor) > DBL_EPSILON ? item.widthFactor : 1;
        if (item.alignmentLength > DBL_EPSILON &&
            textSize.width > DBL_EPSILON) {
            CGFloat targetWidth = item.alignmentLength * scale;
            CGFloat widthCorrection =
                targetWidth / (textSize.width * fabs(drawWidthFactor));
            if (isfinite(widthCorrection) && widthCorrection > DBL_EPSILON) {
                drawWidthFactor *= widthCorrection;
            }
        }
        CGContextSaveGState(context);
        BOOL hasFollowingMasks =
            item.maskingPathStartIndex < drawing.maskingPaths.count;
        if (item.clippingPaths.count || hasFollowingMasks) {
            CGAffineTransform worldToView =
                CGAffineTransformMake(scale, 0, 0, -scale,
                                      offsetX - CGRectGetMinX(drawingBounds) * scale,
                                      offsetY + contentHeight +
                                          CGRectGetMinY(drawingBounds) * scale);
            for (UIBezierPath *clippingPath in item.clippingPaths) {
                CGPathRef viewClipPath =
                    CGPathCreateCopyByTransformingPath(clippingPath.CGPath,
                                                       &worldToView);
                CGContextAddPath(context, viewClipPath);
                if (clippingPath.usesEvenOddFillRule) {
                    CGContextEOClip(context);
                } else {
                    CGContextClip(context);
                }
                CGPathRelease(viewClipPath);
            }
            for (NSUInteger maskIndex = item.maskingPathStartIndex;
                 maskIndex < drawing.maskingPaths.count;
                 maskIndex++) {
                UIBezierPath *maskingPath = drawing.maskingPaths[maskIndex];
                CGPathRef viewMaskPath =
                    CGPathCreateCopyByTransformingPath(maskingPath.CGPath,
                                                       &worldToView);
                CGContextAddRect(context, CGRectInset(self.bounds, -1, -1));
                CGContextAddPath(context, viewMaskPath);
                CGContextEOClip(context);
                CGPathRelease(viewMaskPath);
            }
        }
        CGContextTranslateCTM(context, point.x, point.y);
        CGContextRotateCTM(context, -item.rotation);
        CGContextScaleCTM(context, drawWidthFactor,
                          item.upsideDown ? -1 : 1);
        CGFloat safeObliqueAngle =
            MAX(MIN(item.obliqueAngle, 1.483529864), -1.483529864);
        if (fabs(safeObliqueAngle) > DBL_EPSILON) {
            CGContextConcatCTM(context,
                               CGAffineTransformMake(1, 0,
                                                     tan(safeObliqueAngle), 1,
                                                     0, 0));
        }
        CGRect layoutRect = CGRectMake(originX, originY, layoutWidth, layoutHeight);
        if (item.clipsToBox) CGContextClipToRect(context, layoutRect);
        if (item.verticalText) {
            NSMutableArray<NSString *> *glyphs = [NSMutableArray array];
            [item.text enumerateSubstringsInRange:NSMakeRange(0, item.text.length)
                                          options:NSStringEnumerationByComposedCharacterSequences
                                       usingBlock:^(NSString *substring, NSRange substringRange,
                                                    NSRange enclosingRange, BOOL *stop) {
                if (substring.length && ![substring isEqualToString:@"\n"]) {
                    [glyphs addObject:substring];
                }
            }];
            CGFloat cellHeight =
                MAX(font.lineHeight, JKJDWGMinimumRenderedTextSize);
            CGFloat cellWidth =
                MAX(fontSize, JKJDWGMinimumRenderedTextSize);
            if (localBoxWidth > 0 && localBoxHeight > 0 && glyphs.count) {
                NSUInteger availableRows =
                    MAX((NSUInteger)floor(layoutHeight / cellHeight), 1);
                NSUInteger availableColumns =
                    MAX((NSUInteger)floor(layoutWidth / cellWidth), 1);
                NSUInteger capacity = availableRows * availableColumns;
                if (capacity < glyphs.count) {
                    CGFloat fitScale = sqrt((CGFloat)capacity / glyphs.count) * .95;
                    fontSize =
                        MAX(fontSize * fitScale,
                            JKJDWGMinimumRenderedTextSize);
                    font = JKJDWGFont(item.fontName, fontSize);
                    attributes = @{
                        NSForegroundColorAttributeName:
                            [UIColor colorWithRed:red green:green blue:blue
                                           alpha:MAX(item.alpha, 0.05)],
                        NSFontAttributeName: font
                    };
                    cellHeight =
                        MAX(font.lineHeight,
                            JKJDWGMinimumRenderedTextSize);
                    cellWidth =
                        MAX(fontSize, JKJDWGMinimumRenderedTextSize);
                }
            }
            NSUInteger rowCount = localBoxHeight > 0
                ? MAX((NSUInteger)floor(layoutHeight / cellHeight), 1)
                : MAX(glyphs.count, 1);
            NSUInteger columnCount = MAX((glyphs.count + rowCount - 1) / rowCount, 1);
            if (localBoxWidth <= 0) {
                layoutWidth = columnCount * cellWidth;
                originX = -layoutWidth * item.horizontalAnchor;
            }
            [glyphs enumerateObjectsUsingBlock:^(NSString *glyph,
                                                  NSUInteger glyphIndex,
                                                  BOOL *stop) {
                NSUInteger column = glyphIndex / rowCount;
                NSUInteger row = glyphIndex % rowCount;
                CGFloat glyphX = originX + layoutWidth - (column + 1) * cellWidth;
                CGFloat glyphY = originY + row * cellHeight;
                [glyph drawAtPoint:CGPointMake(glyphX, glyphY) withAttributes:attributes];
            }];
        } else {
            [item.text drawWithRect:layoutRect
                            options:NSStringDrawingUsesLineFragmentOrigin |
                                    NSStringDrawingUsesFontLeading
                         attributes:attributes
                            context:nil];
        }
        CGContextRestoreGState(context);
    }
}

@end

@interface JKJDWGPreviewView () <UIScrollViewDelegate>
@property (nonatomic, strong) UIScrollView *scrollView;
@property (nonatomic, strong) JKJDWGTiledCanvasView *canvasView;
@property (nonatomic, copy) NSArray<JKJDWGDrawing *> *drawings;
@property (nonatomic, strong) UISegmentedControl *spaceSelector;
@property (nonatomic, strong) UIActivityIndicatorView *indicator;
@property (nonatomic, strong) UILabel *messageLabel;
@property (nonatomic) BOOL needsInitialFit;
@property (nonatomic) CGSize lastViewportSize;
@property (nonatomic) CGPoint visibleCanvasCenter;
@property (nonatomic) BOOL hasVisibleCanvasCenter;
@property (nonatomic) BOOL restoringViewportState;
@end

@implementation JKJDWGPreviewView
- (instancetype)init {
    self = [super init];
    if (self) {
        self.backgroundColor = [UIColor colorWithRed:0.075 green:0.086 blue:0.102 alpha:1];
        _scrollView = [UIScrollView new];
        _scrollView.delegate = self;
        _scrollView.minimumZoomScale = JKJDWGMinimumZoomScale;
        _scrollView.maximumZoomScale = 65536;
        _scrollView.backgroundColor = self.backgroundColor;
        _scrollView.translatesAutoresizingMaskIntoConstraints = NO;
        [self addSubview:_scrollView];
        [NSLayoutConstraint activateConstraints:@[
            [_scrollView.topAnchor constraintEqualToAnchor:self.topAnchor],
            [_scrollView.leftAnchor constraintEqualToAnchor:self.leftAnchor],
            [_scrollView.rightAnchor constraintEqualToAnchor:self.rightAnchor],
            [_scrollView.bottomAnchor constraintEqualToAnchor:self.bottomAnchor]
        ]];
        _canvasView = [JKJDWGTiledCanvasView new];
        [_scrollView addSubview:_canvasView];

        _spaceSelector = [UISegmentedControl new];
        _spaceSelector.hidden = YES;
        _spaceSelector.translatesAutoresizingMaskIntoConstraints = NO;
        [_spaceSelector addTarget:self
                           action:@selector(spaceSelectionChanged:)
                 forControlEvents:UIControlEventValueChanged];
        [self addSubview:_spaceSelector];
        [NSLayoutConstraint activateConstraints:@[
            [_spaceSelector.topAnchor constraintEqualToAnchor:self.topAnchor constant:8],
            [_spaceSelector.centerXAnchor constraintEqualToAnchor:self.centerXAnchor],
            [_spaceSelector.widthAnchor constraintLessThanOrEqualToAnchor:self.widthAnchor constant:-32]
        ]];

        _indicator = [[UIActivityIndicatorView alloc] initWithActivityIndicatorStyle:UIActivityIndicatorViewStyleLarge];
        _indicator.color = UIColor.whiteColor;
        _indicator.translatesAutoresizingMaskIntoConstraints = NO;
        [self addSubview:_indicator];
        [_indicator.centerXAnchor constraintEqualToAnchor:self.centerXAnchor].active = YES;
        [_indicator.centerYAnchor constraintEqualToAnchor:self.centerYAnchor].active = YES;
        [_indicator startAnimating];

        _messageLabel = [UILabel new];
        _messageLabel.textColor = UIColor.whiteColor;
        _messageLabel.font = [UIFont systemFontOfSize:15];
        _messageLabel.numberOfLines = 0;
        _messageLabel.textAlignment = NSTextAlignmentCenter;
        _messageLabel.translatesAutoresizingMaskIntoConstraints = NO;
        [self addSubview:_messageLabel];
        [NSLayoutConstraint activateConstraints:@[
            [_messageLabel.centerXAnchor constraintEqualToAnchor:self.centerXAnchor],
            [_messageLabel.centerYAnchor constraintEqualToAnchor:self.centerYAnchor],
            [_messageLabel.leftAnchor constraintGreaterThanOrEqualToAnchor:self.leftAnchor constant:32],
            [_messageLabel.rightAnchor constraintLessThanOrEqualToAnchor:self.rightAnchor constant:-32]
        ]];
        [[NSNotificationCenter defaultCenter] addObserver:self
                                                 selector:@selector(handleMemoryWarning)
                                                     name:UIApplicationDidReceiveMemoryWarningNotification
                                                   object:nil];
    }
    return self;
}

- (void)dealloc {
    [[NSNotificationCenter defaultCenter] removeObserver:self];
    _scrollView.delegate = nil;
}

- (void)handleMemoryWarning {
    CATiledLayer *tiledLayer = (CATiledLayer *)self.canvasView.layer;
    tiledLayer.contents = nil;
    [tiledLayer setNeedsDisplay];
}

- (BOOL)fitDrawingToViewport {
    CGSize viewportSize = self.scrollView.bounds.size;
    CGSize canvasSize = self.canvasView.bounds.size;
    if (viewportSize.width <= DBL_EPSILON ||
        viewportSize.height <= DBL_EPSILON ||
        canvasSize.width <= DBL_EPSILON ||
        canvasSize.height <= DBL_EPSILON) {
        return NO;
    }
    CGFloat fitScale = MIN(viewportSize.width / canvasSize.width,
                           viewportSize.height / canvasSize.height);
    if (!isfinite(fitScale) || fitScale <= DBL_EPSILON) return NO;
    fitScale = MAX(fitScale, JKJDWGMinimumZoomScale);

    // The previous drawing may have installed a larger fit scale as its
    // minimum. Lower it first so UIScrollView will accept the new value.
    self.scrollView.minimumZoomScale = JKJDWGMinimumZoomScale;
    self.scrollView.maximumZoomScale = MAX(65536, fitScale);
    [self.scrollView setZoomScale:fitScale animated:NO];
    self.scrollView.minimumZoomScale = fitScale;
    self.canvasView.viewportZoomScale = fitScale;
    [self updateZoomInsets];
    UIEdgeInsets insets = self.scrollView.contentInset;
    self.scrollView.contentOffset = CGPointMake(-insets.left, -insets.top);
    [self.canvasView.layer setNeedsDisplay];
    return YES;
}

- (void)captureVisibleCanvasCenter {
    if (self.restoringViewportState || !self.canvasView.drawing) return;
    CGSize viewportSize = self.scrollView.bounds.size;
    if (viewportSize.width <= DBL_EPSILON ||
        viewportSize.height <= DBL_EPSILON) {
        return;
    }
    if (self.lastViewportSize.width > DBL_EPSILON &&
        !CGSizeEqualToSize(viewportSize, self.lastViewportSize)) {
        return;
    }
    CGPoint viewportCenter =
        CGPointMake(CGRectGetMidX(self.scrollView.bounds),
                    CGRectGetMidY(self.scrollView.bounds));
    CGPoint canvasCenter =
        [self.canvasView convertPoint:viewportCenter
                             fromView:self.scrollView];
    if (!isfinite(canvasCenter.x) || !isfinite(canvasCenter.y)) return;
    self.visibleCanvasCenter = canvasCenter;
    self.hasVisibleCanvasCenter = YES;
}

- (void)restoreVisibleCanvasCenter:(CGPoint)canvasCenter {
    CGSize canvasSize = self.canvasView.bounds.size;
    canvasCenter.x = MIN(MAX(canvasCenter.x, 0), canvasSize.width);
    canvasCenter.y = MIN(MAX(canvasCenter.y, 0), canvasSize.height);
    CGPoint contentPoint =
        [self.canvasView convertPoint:canvasCenter toView:self.scrollView];
    CGSize viewportSize = self.scrollView.bounds.size;
    CGSize contentSize = self.scrollView.contentSize;
    UIEdgeInsets insets = self.scrollView.contentInset;
    CGFloat minX = -insets.left;
    CGFloat minY = -insets.top;
    CGFloat maxX = MAX(contentSize.width - viewportSize.width + insets.right,
                       minX);
    CGFloat maxY = MAX(contentSize.height - viewportSize.height + insets.bottom,
                       minY);
    CGPoint contentOffset = CGPointMake(
        MIN(MAX(contentPoint.x - viewportSize.width * .5, minX), maxX),
        MIN(MAX(contentPoint.y - viewportSize.height * .5, minY), maxY));
    self.scrollView.contentOffset = contentOffset;
}

- (void)spaceSelectionChanged:(UISegmentedControl *)selector {
    NSInteger index = selector.selectedSegmentIndex;
    if (index < 0 || index >= self.drawings.count) return;
    self.canvasView.drawing = self.drawings[index];
    self.needsInitialFit = YES;
    self.hasVisibleCanvasCenter = NO;
    [self setNeedsLayout];
    [self layoutIfNeeded];
}

- (void)layoutSubviews {
    [super layoutSubviews];
    if (!self.canvasView.drawing) return;
    CGSize viewportSize = self.scrollView.bounds.size;
    if (viewportSize.width <= DBL_EPSILON ||
        viewportSize.height <= DBL_EPSILON) {
        return;
    }
    BOOL viewportSizeChanged =
        self.lastViewportSize.width > DBL_EPSILON &&
        !CGSizeEqualToSize(viewportSize, self.lastViewportSize);
    CGFloat previousFitScale =
        MAX(self.scrollView.minimumZoomScale, JKJDWGMinimumZoomScale);
    CGFloat relativeZoomScale =
        MAX(self.scrollView.zoomScale / previousFitScale, 1);
    CGPoint preservedCanvasCenter = self.visibleCanvasCenter;
    BOOL shouldRestoreCanvasCenter =
        viewportSizeChanged && self.hasVisibleCanvasCenter;

    CGRect drawingBounds = self.canvasView.drawing.drawingBounds;
    CGFloat aspect = CGRectGetWidth(drawingBounds) / CGRectGetHeight(drawingBounds);
    CGFloat width = aspect >= 1 ? 2048 : MAX(2048 * aspect, 1);
    CGFloat height = aspect >= 1 ? MAX(2048 / aspect, 1) : 2048;
    CGRect newFrame = CGRectMake(0, 0, width, height);
    BOOL canvasSizeChanged =
        !CGSizeEqualToSize(self.canvasView.bounds.size, newFrame.size);
    if (canvasSizeChanged) {
        self.restoringViewportState = YES;
        self.scrollView.minimumZoomScale = JKJDWGMinimumZoomScale;
        [self.scrollView setZoomScale:1 animated:NO];
        self.canvasView.frame = newFrame;
        self.scrollView.contentSize = newFrame.size;
        self.restoringViewportState = NO;
        [self.canvasView.layer setNeedsDisplay];
    }
    if (self.needsInitialFit) {
        self.needsInitialFit = NO;
        if (![self fitDrawingToViewport]) {
            self.needsInitialFit = YES;
            [self updateZoomInsets];
        }
    } else if (viewportSizeChanged) {
        CGFloat fitScale = MIN(viewportSize.width / width,
                               viewportSize.height / height);
        fitScale = MAX(fitScale, JKJDWGMinimumZoomScale);
        CGFloat targetZoomScale =
            MIN(MAX(fitScale * relativeZoomScale, fitScale), 65536);
        self.restoringViewportState = YES;
        self.scrollView.minimumZoomScale = JKJDWGMinimumZoomScale;
        self.scrollView.maximumZoomScale = MAX(65536, targetZoomScale);
        [self.scrollView setZoomScale:targetZoomScale animated:NO];
        self.scrollView.minimumZoomScale = fitScale;
        self.canvasView.viewportZoomScale = targetZoomScale;
        [self updateZoomInsets];
        if (shouldRestoreCanvasCenter) {
            [self restoreVisibleCanvasCenter:preservedCanvasCenter];
        }
        self.restoringViewportState = NO;
        [self.canvasView.layer setNeedsDisplay];
    } else {
        [self updateZoomInsets];
    }
    self.lastViewportSize = viewportSize;
    [self captureVisibleCanvasCenter];
}

- (void)loadFile:(NSString *)filePath {
    __weak typeof(self) weakSelf = self;
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
        @autoreleasepool {
            NSError *error = nil;
            NSArray<JKJDWGDrawing *> *drawings =
                [JKJDWGDrawing drawingsWithFile:filePath error:&error];
            dispatch_async(dispatch_get_main_queue(), ^{
                __strong typeof(weakSelf) self = weakSelf;
                if (!self) return;
                [self.indicator stopAnimating];
                if (!drawings.count) {
                    self.messageLabel.text = error.localizedDescription ?: @"DWG 文件无法展示";
                    return;
                }
                self.drawings = drawings;
                [self.spaceSelector removeAllSegments];
                [drawings enumerateObjectsUsingBlock:^(JKJDWGDrawing *drawing,
                                                        NSUInteger index,
                                                        BOOL *stop) {
                    [self.spaceSelector insertSegmentWithTitle:drawing.spaceName
                                                       atIndex:index
                                                      animated:NO];
                }];
                NSUInteger activeIndex =
                    [drawings indexOfObjectPassingTest:^BOOL(JKJDWGDrawing *drawing,
                                                             NSUInteger index,
                                                             BOOL *stop) {
                        return drawing.activeSpace;
                    }];
                if (activeIndex == NSNotFound) activeIndex = 0;
                self.spaceSelector.selectedSegmentIndex = activeIndex;
                self.spaceSelector.hidden = drawings.count < 2;
                self.messageLabel.hidden = YES;
                self.canvasView.drawing = drawings[activeIndex];
                self.needsInitialFit = YES;
                self.hasVisibleCanvasCenter = NO;
                [self setNeedsLayout];
                [self layoutIfNeeded];
            });
        }
    });
}

- (UIView *)viewForZoomingInScrollView:(UIScrollView *)scrollView {
    return self.canvasView;
}

- (void)scrollViewDidZoom:(UIScrollView *)scrollView {
    self.canvasView.viewportZoomScale =
        MAX(scrollView.zoomScale, JKJDWGMinimumZoomScale);
    [self updateZoomInsets];
    [self captureVisibleCanvasCenter];
}

- (void)scrollViewDidScroll:(UIScrollView *)scrollView {
    [self captureVisibleCanvasCenter];
}

- (void)scrollViewDidEndZooming:(UIScrollView *)scrollView
                       withView:(UIView *)view
                        atScale:(CGFloat)scale {
    self.canvasView.viewportZoomScale =
        MAX(scale, JKJDWGMinimumZoomScale);
    [self.canvasView.layer setNeedsDisplay];
    [self captureVisibleCanvasCenter];
}

- (void)updateZoomInsets {
    CGSize viewportSize = self.scrollView.bounds.size;
    CGSize contentSize = self.scrollView.contentSize;
    CGFloat horizontalInset = MAX((viewportSize.width - contentSize.width) * .5, 0);
    CGFloat verticalInset = MAX((viewportSize.height - contentSize.height) * .5, 0);
    self.scrollView.contentInset =
        UIEdgeInsetsMake(verticalInset, horizontalInset,
                         verticalInset, horizontalInset);
}
@end

#endif
