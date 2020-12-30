/*
 * Copyright (c) 2013, Oracle and/or its affiliates. All rights reserved.
 * DO NOT ALTER OR REMOVE COPYRIGHT NOTICES OR THIS FILE HEADER.
 *
 * This code is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 only, as
 * published by the Free Software Foundation.  Oracle designates this
 * particular file as subject to the "Classpath" exception as provided
 * by Oracle in the LICENSE file that accompanied this code.
 *
 * This code is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * version 2 for more details (a copy is included in the LICENSE file that
 * accompanied this code).
 *
 * You should have received a copy of the GNU General Public License version
 * 2 along with this work; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Please contact Oracle, 500 Oracle Parkway, Redwood Shores, CA 94065 USA
 * or visit www.oracle.com if you need additional information or have any
 * questions.
 */
#include "jni.h"
#include <stdlib.h>
#include <stdio.h>
#include <dlfcn.h>
#include <pthread.h>
#include <unistd.h>
// Boardwalk: missing include
#include <string.h>

#include "log.h"

#include "utils.h"

// PojavLancher: fixme: are these wrong?
#define FULL_VERSION "1.16.0-internal"
#define DOT_VERSION "1.16"

static const char* const_progname = "java";
static const char* const_launcher = "openjdk";
static const char** const_jargs = NULL;
static const char** const_appclasspath = NULL;
static const jboolean const_javaw = JNI_FALSE;
static const jboolean const_cpwildcard = JNI_TRUE;
static const jint const_ergo_class = 0; // DEFAULT_POLICY

typedef jint JNI_CreateJavaVM_func(JavaVM **pvm, void **penv, void *args);

typedef jint JLI_Launch_func(int argc, char ** argv, /* main argc, argc */
        int jargc, const char** jargv,          /* java args */
        int appclassc, const char** appclassv,  /* app classpath */
        const char* fullversion,                /* full version defined */
        const char* dotversion,                 /* dot version defined */
        const char* pname,                      /* program name */
        const char* lname,                      /* launcher name */
        jboolean javaargs,                      /* JAVA_ARGS */
        jboolean cpwildcard,                    /* classpath wildcard*/
        jboolean javaw,                         /* windows-only javaw */
        jint ergo                               /* ergonomics class policy */
);

static jint launchJVM(int margc, char** margv) {
   void* libjli = dlopen("/usr/lib/jvm/java-16-openjdk/lib/libjli.dylib", RTLD_LAZY | RTLD_GLOBAL);
   
   // Boardwalk: silence
   // LOGD("JLI lib = %x", (int)libjli);
   if (NULL == libjli) {
       printf("JLI lib = NULL: %s\n", dlerror());
       return -1;
   }
   printf("Found JLI lib\n");

   JLI_Launch_func *pJLI_Launch =
          (JLI_Launch_func *)dlsym(libjli, "JLI_Launch");
    // Boardwalk: silence
    // LOGD("JLI_Launch = 0x%x", *(int*)&pJLI_Launch);

   if (NULL == pJLI_Launch) {
       printf("JLI_Launch = NULL\n");
       return -1;
   }

   printf("Calling JLI_Launch\n");

   return pJLI_Launch(margc, margv,
                   0, NULL, // sizeof(const_jargs) / sizeof(char *), const_jargs,
                   0, NULL, // sizeof(const_appclasspath) / sizeof(char *), const_appclasspath,
                   FULL_VERSION,
                   DOT_VERSION,
                   *margv, // (const_progname != NULL) ? const_progname : *margv,
                   *margv, // (const_launcher != NULL) ? const_launcher : *margv,
                   (const_jargs != NULL) ? JNI_TRUE : JNI_FALSE,
                   const_cpwildcard, const_javaw, const_ergo_class);
}

/*
 * Class:     com_oracle_dalvik_VMLauncher
 * Method:    launchJVM
 * Signature: ([Ljava/lang/String;)I
 */
JNIEXPORT jint JNICALL Java_com_oracle_dalvik_VMLauncher_launchJVM(JNIEnv *env, jclass clazz, jobjectArray argsArray) {
   jint res = 0;
   // int i;

    // Save dalvik JNIEnv pointer for JVM launch thread
    dalvikJNIEnvPtr_ANDROID = env;

    if (argsArray == NULL) {
        printf("Args array null, returning\n");
        //handle error
        return 0;
    }

    int argc = (*env)->GetArrayLength(env, argsArray);
    char **argv = convert_to_char_array(env, argsArray);
    
    printf("Done processing args\n");

    res = launchJVM(argc, argv);

    printf("Going to free args\n");
    free_char_array(env, argsArray, argv);
    
    printf("Free done\n");
   
    return res;
}

JNIEXPORT void JNICALL
Java_net_kdt_pojavlaunch_utils_JREUtils_redirectLogcat(JNIEnv *env, jclass clazz, jstring path) {
    char *path_c = (*env)->GetStringUTFChars(env, path, 0);
    FILE* logFile = fopen(path_c, "w");
    (*env)->ReleaseStringUTFChars(env, path, path_c);

    int fd = fileno(logFile);

    dup2(fd, 1);
    dup2(fd, 2);

    close(fd);

    printf("Starting logging STDIO as jrelog:V\n");
}

