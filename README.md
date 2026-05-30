<p align="center">
  <a href="https://rux-lang.dev" target="_blank">
    <img src="https://rux-lang.dev/logo.svg" alt="Rux Logo" width="150"/>
  </a>
</p>

# Rux Programming Language

[![GitHub stars][GitHubStarBadge]][GitHubUrl]
[![GitHub followers][GitHubFollowerBadge]][GitHubUrl]
[![GitHub forks][GitHubForkBadge]][GitHubUrl]
[![Discussion][DiscussionBadge]][DiscussionUrl]
[![Discord][DiscordBadge]][DiscordUrl]
[![Reddit][RedditBadge]][RedditUrl]
[![YouTube][YouTubeBadge]][YouTubeUrl]
[![X][XBadge]][XUrl]
[![Bluesky][BlueskyBadge]][BlueskyUrl]
[![Mastodon][MastodonBadge]][MastodonUrl]
[![Telegram][TelegramBadge]][TelegramUrl]
[![License][LicenseBadge]][LicenseUrl]

[![Linux](https://github.com/rux-lang/Rux/actions/workflows/linux.yml/badge.svg)](https://github.com/rux-lang/Rux/actions/workflows/linux.yml)
[![macOS](https://github.com/rux-lang/Rux/actions/workflows/macos.yml/badge.svg)](https://github.com/rux-lang/Rux/actions/workflows/macos.yml)
[![Windows](https://github.com/rux-lang/Rux/actions/workflows/windows.yml/badge.svg)](https://github.com/rux-lang/Rux/actions/workflows/windows.yml)

Rux is a fast, compiled, strongly typed, multi-paradigm, general-purpose programming language.

## Project Status

Currently, under development.

## Documentation

- [Language Reference](https://rux-lang.dev/docs)
- [Library API Reference](https://rux-lang.dev/api)
- [CLI Reference](https://rux-lang.dev/cli)
- [Tutorials](https://rux-lang.dev/tutorials)

## Community

Here’s how you can get involved:

- Join the conversation on [GitHub Discussions](https://github.com/rux-lang/rux/discussions), [Discord](https://discord.com/invite/uvSHjtZSVG), or [Reddit](https://www.reddit.com/r/ruxlang)
- Subscribe on [YouTube](https://www.youtube.com/@ruxlang), [X](https://x.com/ruxlang), [Bluesky](https://bsky.app/profile/ruxlang.bsky.social), [Mastodon](https://mastodon.social/@ruxlang), [Telegram](https://t.me/ruxlang) to get early updates, dev logs, and sneak peeks
- Contribute ideas — from grammar tweaks to mascot variants, we’re open to playful and technical input alike
- Discuss architecture — compiler design, type systems, and extensibility are all on the table

## Building

### Prerequisites

- [CMake](https://cmake.org/) 4.2 or later
- A C++26-capable compiler (e.g. Clang 19+, GCC 14+, MSVC 2022+)
- A build tool supported by CMake, such as Ninja, Make, or MSBuild

### Optional: LLVM Backend

To build with the LLVM backend (for multi-architecture support and optimizations):

- [LLVM](https://llvm.org/) 19 or later
- LLVM development libraries and headers
- On Ubuntu/Debian: `sudo apt install llvm-19-dev libllvm-19-dev`
- On macOS: `brew install llvm@19`
- On Windows: Download LLVM from [llvm.org](https://llvm.org/) and set `LLVM_DIR` environment variable

### Clone

```sh
git clone https://github.com/rux-lang/Rux.git
cd Rux
```

### Build with Clang

Use this configuration when `clang++` is available on your `PATH`.

```sh
cmake -S . -B build/clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release
cmake --build build/clang --config Release
```

### Build with LLVM Backend

To enable the LLVM backend for multi-architecture support and optimizations:

```sh
cmake -S . -B build/llvm -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release -DUSE_LLVM_BACKEND=ON
cmake --build build/llvm --config Release
```

**Windows with MinGW and LLVM:**

If you have LLVM installed via MSYS2/MinGW, you can build with the LLVM backend using:

```sh
cmake -S . -B build/mingw -G Ninja -DUSE_LLVM_BACKEND=ON -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_C_COMPILER=clang -DCMAKE_BUILD_TYPE=Release -DLLVM_DIR=C:/msys64/mingw64/lib/cmake/llvm
cmake --build build/mingw --config=Release
```

**Note:** If you have multiple LLVM installations in your PATH (e.g., from MSYS2/MinGW and a separate LLVM installer), the Windows DLL loader may load the wrong LLVM DLL. To fix this, ensure MinGW64 bin is prioritized in your PATH when running the executable:

```sh
export PATH=/c/msys64/mingw64/bin:$PATH
./build/mingw/rux.exe help
```

If you use Ninja explicitly:

```sh
cmake -S . -B build/clang -G Ninja -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=Release
cmake --build build/clang
```

On Windows, Clang can also be used through `clang-cl` from a Visual Studio Developer PowerShell:

```sh
cmake -S . -B build/clang-cl -G "Visual Studio 17 2022" -T ClangCL
cmake --build build/clang-cl --config Release
```

### Build with MSVC

Run these commands from a Visual Studio Developer PowerShell or Developer Command Prompt:

```sh
cmake -S . -B build/msvc -G "Visual Studio 17 2022" -A x64
cmake --build build/msvc --config Release
```

Alternatively, with Ninja and the MSVC compiler environment already loaded:

```sh
cmake -S . -B build/msvc-ninja -G Ninja -DCMAKE_CXX_COMPILER=cl -DCMAKE_BUILD_TYPE=Release
cmake --build build/msvc-ninja
```

### Generic CMake Build

If your preferred compiler is already the default for your environment:

```sh
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

The `rux` binary will be placed under the selected build directory. Single-configuration generators usually emit it directly in that directory, while Visual Studio/MSVC multi-configuration builds emit it under `Release/`.

To install it onto your system:

```sh
cmake --install build --prefix /usr/local
```

### macOS notes

On macOS, `rux` produces statically linked, ad-hoc code-signed Mach-O
executables. Generated programs are currently **x86-64 only** — on Apple
Silicon they run through Rosetta 2 (the `rux` compiler itself builds and runs
natively on arm64). A Homebrew formula for distributing `rux` lives under
[`packaging/homebrew/`](packaging/homebrew/).

## Contributing

The Rux repository is hosted at [rux-lang/Rux](https://github.com/rux-lang/Rux) on GitHub.

Read the [Contributing guide](CONTRIBUTING.md) to get started.

## License

[MIT](LICENSE)

[GitHubStarBadge]: https://img.shields.io/github/stars/rux-lang/Rux?style=flat&logo=github&label=Stars&logoColor=white&color=blue
[GitHubFollowerBadge]: https://img.shields.io/github/followers/rux-lang?style=flat&logo=github&label=Followers&logoColor=white&color=blue
[GitHubForkBadge]: https://img.shields.io/github/forks/rux-lang/Rux?style=flat&logo=github&logoColor=white&label=Forks&color=blue
[DiscussionBadge]: https://img.shields.io/badge/Discussions-gray?logo=github
[DiscordBadge]: https://img.shields.io/discord/1321469752811585576?style=flat&logo=discord&logoColor=white&label=Discord&color=blue
[RedditBadge]: https://img.shields.io/reddit/subreddit-subscribers/ruxlang?style=flat&logo=reddit&logoColor=white&label=Reddit&color=blue
[YouTubeBadge]: https://img.shields.io/youtube/channel/subscribers/UCNqQ7NIA5pBl3ZO--nOyvDA?style=flat&logo=youtube&logoColor=white&label=YouTube&color=blue
[BlueskyBadge]: https://img.shields.io/bluesky/followers/ruxlang.bsky.social?style=flat&logo=Bluesky&logoColor=white&label=Bluesky&color=blue
[MastodonBadge]: https://img.shields.io/mastodon/follow/113727153489087809?domain=mastodon.social&style=flat&logo=mastodon&logoColor=white&label=Mastodon&color=blue
[XBadge]: https://img.shields.io/badge/11-gray?logo=x&style=flat&logoColor=white&label=Twitter&color=blue
[TelegramBadge]: https://img.shields.io/badge/10-gray?style=flat&logo=telegram&logoColor=white&label=Telegram&color=blue
[LicenseBadge]: https://img.shields.io/github/license/rux-lang/Rux?style=flat
[GitHubUrl]: https://github.com/rux-lang
[DiscussionUrl]: https://github.com/rux-lang/Rux/discussions
[DiscordUrl]: https://discord.com/invite/uvSHjtZSVG
[RedditUrl]: https://www.reddit.com/r/ruxlang
[YouTubeUrl]: https://www.youtube.com/@ruxlang
[XUrl]: https://x.com/ruxlang
[BlueskyUrl]: https://bsky.app/profile/ruxlang.bsky.social
[MastodonUrl]: https://mastodon.social/@ruxlang
[TelegramUrl]: https://t.me/ruxlang
[LicenseUrl]: https://github.com/rux-lang/Rux/blob/main/LICENSE
