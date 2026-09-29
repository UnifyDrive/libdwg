#import "ViewController.h"
#import "L10n.h"

#define JKJ_DWG_PREVIEW_IMPLEMENTATION 1
#import "../Universal/Modules/DocPreview/View/JKJDWGPreviewView.h"

@interface ViewController () <UIDocumentPickerDelegate>

@property (nonatomic, strong) UILabel *emptyLabel;
@property (nonatomic, strong) UIButton *openButton;
@property (nonatomic, strong) JKJDWGPreviewView *previewView;

@end

@implementation ViewController

- (void)viewDidLoad {
    [super viewDidLoad];
    self.title = L10n.appTitle;
    self.view.backgroundColor = UIColor.systemBackgroundColor;

    self.navigationItem.rightBarButtonItem =
        [[UIBarButtonItem alloc] initWithTitle:L10n.openFile
                                         style:UIBarButtonItemStylePlain
                                        target:self
                                        action:@selector(openDocumentPicker)];

    UILabel *emptyLabel = [UILabel new];
    emptyLabel.translatesAutoresizingMaskIntoConstraints = NO;
    emptyLabel.text = L10n.emptyHint;
    emptyLabel.textColor = UIColor.secondaryLabelColor;
    emptyLabel.textAlignment = NSTextAlignmentCenter;
    emptyLabel.numberOfLines = 0;
    [self.view addSubview:emptyLabel];
    [NSLayoutConstraint activateConstraints:@[
        [emptyLabel.leadingAnchor constraintGreaterThanOrEqualToAnchor:self.view.leadingAnchor constant:24],
        [emptyLabel.trailingAnchor constraintLessThanOrEqualToAnchor:self.view.trailingAnchor constant:-24],
        [emptyLabel.centerXAnchor constraintEqualToAnchor:self.view.centerXAnchor],
        [emptyLabel.centerYAnchor constraintEqualToAnchor:self.view.centerYAnchor],
    ]];
    self.emptyLabel = emptyLabel;

    UIButton *openButton = [UIButton buttonWithType:UIButtonTypeSystem];
    openButton.translatesAutoresizingMaskIntoConstraints = NO;
    openButton.titleLabel.font = [UIFont preferredFontForTextStyle:UIFontTextStyleHeadline];
    openButton.contentEdgeInsets = UIEdgeInsetsMake(12, 24, 12, 24);
    openButton.layer.cornerRadius = 10;
    openButton.layer.borderWidth = 1;
    openButton.layer.borderColor = UIColor.systemBlueColor.CGColor;
    [openButton setTitle:L10n.openFile forState:UIControlStateNormal];
    [openButton addTarget:self
                   action:@selector(openDocumentPicker)
         forControlEvents:UIControlEventTouchUpInside];
    [self.view addSubview:openButton];
    [NSLayoutConstraint activateConstraints:@[
        [openButton.topAnchor constraintEqualToAnchor:emptyLabel.bottomAnchor constant:20],
        [openButton.centerXAnchor constraintEqualToAnchor:self.view.centerXAnchor],
    ]];
    self.openButton = openButton;
}

- (void)openDocumentPicker {
    UIDocumentPickerViewController *picker =
        [[UIDocumentPickerViewController alloc] initWithDocumentTypes:@[@"com.autodesk.dwg", @"public.data"]
                                                               inMode:UIDocumentPickerModeOpen];
    picker.delegate = self;
    picker.allowsMultipleSelection = NO;
    [self presentViewController:picker animated:YES completion:nil];
}

- (void)documentPicker:(UIDocumentPickerViewController *)controller
 didPickDocumentsAtURLs:(NSArray<NSURL *> *)urls {
    NSURL *URL = urls.firstObject;
    if (!URL) return;

    BOOL accessing = [URL startAccessingSecurityScopedResource];
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
        __block NSURL *localURL = nil;
        __block NSError *copyError = nil;
        NSError *coordinationError = nil;
        NSFileCoordinator *coordinator = [[NSFileCoordinator alloc] initWithFilePresenter:nil];

        [coordinator coordinateReadingItemAtURL:URL
                                        options:NSFileCoordinatorReadingWithoutChanges
                                          error:&coordinationError
                                     byAccessor:^(NSURL *newURL) {
            NSURL *importRoot =
                [[NSURL fileURLWithPath:NSTemporaryDirectory() isDirectory:YES]
                    URLByAppendingPathComponent:@"ImportedDWG"
                                    isDirectory:YES];
            NSURL *importDirectory =
                [importRoot URLByAppendingPathComponent:NSUUID.UUID.UUIDString
                                            isDirectory:YES];
            NSFileManager *fileManager = NSFileManager.defaultManager;
            if (![fileManager createDirectoryAtURL:importDirectory
                       withIntermediateDirectories:YES
                                        attributes:nil
                                             error:&copyError]) {
                return;
            }

            NSString *fileName = newURL.lastPathComponent.length
                ? newURL.lastPathComponent : @"drawing.dwg";
            NSURL *destinationURL =
                [importDirectory URLByAppendingPathComponent:fileName isDirectory:NO];
            if ([fileManager copyItemAtURL:newURL
                                     toURL:destinationURL
                                     error:&copyError]) {
                localURL = destinationURL;
            }
        }];

        if (accessing) [URL stopAccessingSecurityScopedResource];
        NSError *error = copyError ?: coordinationError;
        dispatch_async(dispatch_get_main_queue(), ^{
            if (localURL) {
                [self showDWGAtPath:localURL.path];
            } else {
                self.emptyLabel.hidden = NO;
                self.openButton.hidden = NO;
                self.emptyLabel.text = error.localizedDescription ?: L10n.emptyHint;
            }
        });
    });
}

- (void)showDWGAtPath:(NSString *)filePath {
    [self.previewView removeFromSuperview];

    JKJDWGPreviewView *previewView = [JKJDWGPreviewView new];
    previewView.translatesAutoresizingMaskIntoConstraints = NO;
    [self.view addSubview:previewView];
    [NSLayoutConstraint activateConstraints:@[
        [previewView.topAnchor constraintEqualToAnchor:self.view.safeAreaLayoutGuide.topAnchor],
        [previewView.leadingAnchor constraintEqualToAnchor:self.view.leadingAnchor],
        [previewView.trailingAnchor constraintEqualToAnchor:self.view.trailingAnchor],
        [previewView.bottomAnchor constraintEqualToAnchor:self.view.bottomAnchor],
    ]];

    self.emptyLabel.hidden = YES;
    self.openButton.hidden = YES;
    self.previewView = previewView;
    [previewView loadFile:filePath];
}

@end
