#import "SceneDelegate.h"
#import "ViewController.h"

@implementation SceneDelegate

- (void)scene:(UIScene *)scene
    willConnectToSession:(UISceneSession *)session
    options:(UISceneConnectionOptions *)connectionOptions {
    if (![scene isKindOfClass:UIWindowScene.class]) return;

    self.window = [[UIWindow alloc] initWithWindowScene:(UIWindowScene *)scene];
    ViewController *viewController = [ViewController new];
    self.window.rootViewController =
        [[UINavigationController alloc] initWithRootViewController:viewController];
    [self.window makeKeyAndVisible];
}

@end
