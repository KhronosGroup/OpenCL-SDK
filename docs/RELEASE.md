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

```
git tag vYYYY.MM.DD
git push <remote_name> vYYYY.MM.DD
```