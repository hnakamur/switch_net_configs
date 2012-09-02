#include <SystemConfiguration/SystemConfiguration.h>
#include <CoreFoundation/CFStringEncodingExt.h>


// Error Handling
// --------------
// SCF returns errors in two ways:
//
// o The function result is usually set to something
//   generic (like NULL or false) to indicate an error.
//
// o There is a call, SCError, that returns the error
//   code for the most recent function. These error codes
//   are not in the OSStatus domain.
//
// We deal with this using two functions, MoreSCError
// and MoreSCErrorBoolean. Both of these take a generic
// failure indicator (a pointer or a Boolean) and, if
// that indicates an error, they call SCError to get the
// real error code. They also act as a bottleneck for
// mapping SC errors into the OSStatus domain, although
// I don't do that in this simple implementation.
//
// Note that I could have eliminated the failure indicator
// parameter and just called SCError but I'm worried
// about SCF returning an error indicator without setting
// the SCError. There's no justification for this worry
// other than general paranoia (I know of no examples where
// this happens),

static OSStatus MoreSCErrorBoolean(Boolean success)
{
    OSStatus err;
    int scErr;

    err = noErr;
    if ( ! success ) {
        scErr = SCError();
        if (scErr == kSCStatusOK) {
            scErr = kSCStatusFailed;
        }
        // Return an SCF error directly as an OSStatus.
        // That's a little cheesy. In a real program
        // you might want to do some mapping from SCF
        // errors to a range within the OSStatus range.
        err = scErr;
    }
    return err;
}

static OSStatus MoreSCError(const void *value)
{
    return MoreSCErrorBoolean(value != NULL);
}

static OSStatus CFQError(CFTypeRef cf)
    // Maps Core Foundation error indications (such as they
    // are) to the OSStatus domain.
{
    OSStatus err;

    err = noErr;
    if (cf == NULL) {
        err = -1; //coreFoundationUnknownErr;
    }
    return err;
}

static void CFQRelease(CFTypeRef cf)
    // A version of CFRelease that's tolerant of NULL.
{
    if (cf != NULL) {
        CFRelease(cf);
    }
}



static OSStatus CreateIPAddressListChangeCallbackSCF(
                        SCDynamicStoreCallBack callback,
                        void *contextPtr,
                        SCDynamicStoreRef *storeRef,
                        CFRunLoopSourceRef *sourceRef)
    // Create a SCF dynamic store reference and a
    // corresponding CFRunLoop source. If you add the
    // run loop source to your run loop then the supplied
    // callback function will be called when local IP
    // address list changes.
{
    OSStatus                err;
    SCDynamicStoreContext   context = {0, NULL, NULL, NULL, NULL};
    SCDynamicStoreRef       ref;
    CFStringRef             pattern;
    CFArrayRef              patternList;
    CFRunLoopSourceRef      rls;

    assert(callback   != NULL);
    assert( storeRef  != NULL);
    assert(*storeRef  == NULL);
    assert( sourceRef != NULL);
    assert(*sourceRef == NULL);

    ref = NULL;
    pattern = NULL;
    patternList = NULL;
    rls = NULL;

    // Create a connection to the dynamic store, then create
    // a search pattern that finds all IPv4 entities.
    // The pattern is "State:/Network/Service/[^/]+/IPv4".

    context.info = contextPtr;
    ref = SCDynamicStoreCreate( NULL,
                                CFSTR("AddIPAddressListChangeCallbackSCF"),
                                callback,
                                &context);
    err = MoreSCError(ref);
    if (err == noErr) {
        pattern = SCDynamicStoreKeyCreateNetworkServiceEntity(
                                NULL,
                                kSCDynamicStoreDomainState,
                                kSCCompAnyRegex,
                                kSCEntNetIPv4);
        err = MoreSCError(pattern);
    }

    // Create a pattern list containing just one pattern,
    // then tell SCF that we want to watch changes in keys
    // that match that pattern list, then create our run loop
    // source.

    if (err == noErr) {
        patternList = CFArrayCreate(NULL,
                                    (const void **) &pattern, 1,
                                    &kCFTypeArrayCallBacks);
        err = CFQError(patternList);
    }
    if (err == noErr) {
        err = MoreSCErrorBoolean(
                SCDynamicStoreSetNotificationKeys(
                    ref,
                    NULL,
                    patternList)
              );
    }
    if (err == noErr) {
        rls = SCDynamicStoreCreateRunLoopSource(NULL, ref, 0);
        err = MoreSCError(rls);
    }

    // Clean up.

    CFQRelease(pattern);
    CFQRelease(patternList);
    if (err != noErr) {
        CFQRelease(ref);
        ref = NULL;
    }
    *storeRef = ref;
    *sourceRef = rls;

    assert( (err == noErr) == (*storeRef  != NULL) );
    assert( (err == noErr) == (*sourceRef != NULL) );

    return err;
}

static char *command;

static void IPConfigChangedCallback(SCDynamicStoreRef store, CFArrayRef changedKeys, void * info)
{
/*
   CFIndex count = CFArrayGetCount(changedKeys);
   CFIndex i;

   char **argv = (char **)malloc(sizeof(char *) * (1 + count + 1));

   argv[0] = "./a.sh";
   printf("IP Configuration changed, do something!\n");
   for (i = 0; i < count; ++i) {
     CFStringRef key = (CFStringRef)CFArrayGetValueAtIndex(changedKeys, i);
     const char *str = CFStringGetCStringPtr(key, kCFStringEncodingUTF8);
     argv[i + 1] = (char *)str;
     printf("key=%s\n", str);
   }
   argv[count + 1] = NULL;

   int ret = execve("./a.sh", argv, NULL);
   if (ret == -1) {
     printf("execve failed\n");
   }

   free(argv);
*/

   int ret = system(command);
   if (ret == -1) {
     fprintf(stderr, "Cannot execute %s\n", command);
   }
}

volatile bool _threadKeepGoing = true;  // in real life another thread might tell us to quit by setting this false and then calling CFRunLoopStop() on our CFRunLoop()

int main(int argc, char **argv)
{
   if (argc != 2) {
     fprintf(stderr, "Usage: %s command\n", argv[0]);
     exit(1);
   }

   command = argv[1];

   void * contextPtr = NULL;  // only NULL for this trivial example; in real life you could point this to some data you want the IPConfigChangedCallback() to have access to
   SCDynamicStoreRef storeRef = NULL;
   CFRunLoopSourceRef sourceRef = NULL;
   if (CreateIPAddressListChangeCallbackSCF(IPConfigChangedCallback, contextPtr, &storeRef, &sourceRef) == noErr)
   {
      CFRunLoopAddSource(CFRunLoopGetCurrent(), sourceRef, kCFRunLoopDefaultMode);
      while(_threadKeepGoing) CFRunLoopRun();
      CFRunLoopRemoveSource(CFRunLoopGetCurrent(), sourceRef, kCFRunLoopDefaultMode);
      CFRelease(storeRef);
      CFRelease(sourceRef);
   }
}
