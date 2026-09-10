plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

android {
    namespace = "com.pocketengineer.app"
    compileSdk = 35
    ndkVersion = "26.3.11579264"

    defaultConfig {
        applicationId = "com.pocketengineer.app"
        minSdk = 24
        targetSdk = 35
        versionCode = 5
        versionName = "0.5.0"
        testInstrumentationRunner = "androidx.test.runner.AndroidJUnitRunner"

        externalNativeBuild {
            cmake {
                cppFlags += listOf("-std=c++20")
                // A single JNI shared library owns the entire C++ runtime.
                // Avoid the 4KB-aligned libc++_shared from older NDKs.
                arguments += listOf("-DANDROID_STL=c++_static")
            }
        }
    }

    val releaseKey = System.getenv("PE_ANDROID_KEYSTORE")
    testBuildType = providers.gradleProperty("peTestBuildType").getOrElse("debug")
    signingConfigs {
        if (!releaseKey.isNullOrBlank()) {
            create("projectRelease") {
                storeFile = file(releaseKey)
                storePassword = System.getenv("PE_ANDROID_STORE_PASSWORD")
                keyAlias = System.getenv("PE_ANDROID_KEY_ALIAS")
                keyPassword = System.getenv("PE_ANDROID_KEY_PASSWORD")
            }
        }
    }
    buildTypes {
        release {
            isMinifyEnabled = false
            if (!releaseKey.isNullOrBlank()) signingConfig = signingConfigs.getByName("projectRelease")
            else if (System.getenv("PE_ALLOW_DEVELOPMENT_SIGNATURE") == "true") signingConfig = signingConfigs.getByName("debug")
        }
    }

    compileOptions {
        sourceCompatibility = JavaVersion.VERSION_17
        targetCompatibility = JavaVersion.VERSION_17
    }

    kotlinOptions {
        jvmTarget = "17"
    }

    externalNativeBuild {
        cmake {
            path = file("src/main/cpp/CMakeLists.txt")
            version = "3.22.1"
        }
    }

    sourceSets {
        getByName("main").assets.srcDir("../../www")
    }
}

dependencies {
    implementation("androidx.webkit:webkit:1.12.1")
    androidTestImplementation("androidx.test:runner:1.6.2")
    androidTestImplementation("androidx.test.ext:junit:1.2.1")
    androidTestImplementation("androidx.test:rules:1.6.1")
    androidTestImplementation("androidx.test.uiautomator:uiautomator:2.3.0")
}
