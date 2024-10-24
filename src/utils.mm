#ifdef DM_PLATFORM_IOS
#include <dmsdk/sdk.h>
#include "UIKit/UIKit.h"
void DoHapticFeedbackInternal() {
	UIImpactFeedbackGenerator *H = [[UIImpactFeedbackGenerator alloc] initWithStyle:UIImpactFeedbackStyleMedium];
	                          [H prepare];         [H impactOccurred];                                 /* ARC */
}
#endif