#ifdef DM_PLATFORM_IOS
#include "UIKit/UIKit.h"

void doHapticFeedbackInternal() {
	UIImpactFeedbackGenerator
    *H = [[UIImpactFeedbackGenerator alloc] initWithStyle:UIImpactFeedbackStyleMedium];
	[H prepare];         [H impactOccurred];                                  /* ARC */
}

#endif