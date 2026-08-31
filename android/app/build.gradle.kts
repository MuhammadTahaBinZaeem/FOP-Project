plugins {
    id("com.android.application")
    id("org.jetbrains.kotlin.android")
}

android {
    namespace = "com.pocketengineer.app"
    compileSdk = 35

    defaultConfig {
        applicationId = "com.pocketengineer.app"
        minSdk = 24
        targetSdk = 35
        versionCode = 1
        versionName = "0.2.0"

        externalNativeBuild {
            cmake {
                cppFlags += listOf("-std=c++20")
            }
        }
    }

    buildTypes {
        release {
            isMinifyEnabled = false
        }
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
