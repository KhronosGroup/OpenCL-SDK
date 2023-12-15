# Release process

To release a new version of the SDK, follow the process in a recursively cloned repo:

> Throughout the guide `<remote_name>` refers to the https://github.com/KhronosGroup hosted remotes. Your dev environment may not refer to these as `origin` but any other name, for eg. `upstream`. Update commands as needed.

## Checkout tip of main

With a clean workspace and empty stage, issue the following.

```
git pull <remote_name>
git checkout <remote_name>/main
```

## Update submodules

Should tip of main have updated submodule remotes, the SDK needs those changes. (This part likely does nothing.)

```
git submodule update --remote
```

## Tag OpenCL projects

In the external folder, tag the OpenCL-related projects with the current date using `vYYYY.MM.DD` format in dep order.

1. OpenCL-Headers
2. OpenCL-ICD-Loader
3. OpenCL-CLHPP

> Note: the aim is that OpenCL-Layers in time will become a submodule/component of the SDK

In each of these repos, issue:

```
git pull origin
git checkout origin/main
git tag vYYYY.MM.DD
git push origin vYYYY.MM.DD
```

> Note 1: Remote name `origin` isn't an oversight, the default remote name for submodules is `origin`.
>
> Note 2: Compatibility between packages is guaranteed manually. CI for each project fetches newest `main` and not using the same tag. Pushing tags in dep order is important to guarantee that when CI runs on pushing tags in these repos, tests are run using the correct versions of their deps.

### Uploading source packages to the Launchpad PPA

As part of the release workflow for the sub-projects, a Debian source package is created and uploaded to the prescribed PPA. The details of the packaging must be set up in the GitHub projects of each sub-project separately. The following values must be set up prior to launching the release workflow:

|Setting type   |Setting name        |Example value                    |Note                                                                  |
|------------   |------------        |-------------                    |----                                                                  |
|Action variable|`DEB_MAINTAINER`    |`Test User <test.user@test.org>` |                                                                      |
|Action variable|`DEB_VERSION_SUFFIX`|`ppa0`                           |                                                                      |
|Action variable|`PPA`               |`KhronosGroup/OpenCL`            |Has to be created on [launchpad.net](https://launchpad.net) beforehand|
|Action secret  |`DEB_SIGNING_KEY`   |`BEGIN PGP PRIVATE KEY BLOCK` ...|Output of `gpg --armor --export-secret-keys <key ID>`                 |


Be aware, that the automatic process of publishing of the binary Debian packages on the PPA can take hours. Moreover, since the projects depend on each other, the person creating the releases **must trigger the release workflow once the binary packages from the prerequisites are live**. The source package dependencies are the following:

|Project                                                               |Dependencies|
|-------                                                               |------------|
|[OpenCL-Headers](https://github.com/KhronosGroup/OpenCL-Headers)      |-|
|[OpenCL-ICD-Loader](https://github.com/KhronosGroup/OpenCL-ICD-Loader)|[OpenCL-Headers](https://github.com/KhronosGroup/OpenCL-Headers)|
|[OpenCL-CLHPP](https://github.com/KhronosGroup/OpenCL-CLHPP)          |[OpenCL-Headers](https://github.com/KhronosGroup/OpenCL-Headers)|
|[OpenCL-SDK](https://github.com/KhronosGroup/OpenCL-SDK)              |[OpenCL-Headers](https://github.com/KhronosGroup/OpenCL-Headers), [OpenCL-ICD-Loader](https://github.com/KhronosGroup/OpenCL-ICD-Loader), [OpenCL-CLHPP](https://github.com/KhronosGroup/OpenCL-CLHPP)|

## Update submodule hashes

Submodules may have moved to a different commit hash due to the previous step. The SDK wants to pick up all those changes (if it hasn't already been done). If `git status` shows, changes, push the changes.

```
git commit -a -m "Bump submodules hashes"
```

## Update project version

Update the project version in CMake

```cmake
project(OpenCL-SDK
  VERSION YYYY.MM.DD
```

```
git commit -a -m "Update project version"
```

## Tag SDK

The automatic release pipeline that is triggered on git tags, generates a Debian source package from the SDK code and uploads it to Launchpad. The packaging details and credentials have to be set as [described before](#uploading-source-packages-to-the-launchpad-ppa).

While the SDK repository consumes the [OpenCL-Headers](https://github.com/KhronosGroup/OpenCL-Headers), [OpenCL-ICD-Loader](https://github.com/KhronosGroup/OpenCL-ICD-Loader) and [OpenCL-CLHPP](https://github.com/KhronosGroup/OpenCL-CLHPP) as git submodules, the same relation between the generated source packages is defined as a package dependency. Therefore, all 3 dependencies must be available in the Launchpad PPA to be able to build the SDK source package. The maintainer must make sure that the dependencies have already become available in the prescribed PPA before attempting to push a release tag for the OpenCL SDK.

All Debian packages are version locked, which means that the **OpenCL SDK Debian package depends on exactly the same versions of the Headers, CLHPP and ICD-Loader as itself**. The source of this version information is the main `project` statement in the CMake scripts, therefore the maintainer has to make sure that all repositories have updated the version number, as [described above](#update-project-version).

When all dependencies are published to the PPA, the SDK can be released by the following commands:

```
git tag vYYYY.MM.DD
git push <remote_name> vYYYY.MM.DD
```