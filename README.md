# Phaka Service Wrapper

A native windows service wrapper that can run applications as services.

## Getting Started

Familiarize yourself with the [documention](docs/index.md).  

### Prerequisites

You'll need the following

- An application you'd like to run as a service, `hello.exe` for example, stored on the local computer. We'll use `c:\tools` as an example.

- Any supported Windows platform. This has been tested on Windows 10, Windows Server 2012 R2 and Windows Server 2016.

### Installing

Download the [latest version](https://github.com/Phaka/Windows-Service-Wrapper/releases) of `wrapper.exe` for your platform to the same location of the application. 

> It is recommended that you rename `wrapper.exe` to something that better suites your purpose as this will reduce confusion for users and security monitoring systems when there are multiple copies used on a computer. For the purpose of this document, we'll use with `wrapper.exe` and you'll need to rename it accordingly.  

In the same directory as `wrapper.exe`, create a configuration file `wrapper.cfg` with the following contents. 

```
[Unit]
Name=phaka-hello-service
CommandLine=hello.exe
Title=Phaka Hello Service
Description=Provides secure storage and retrieval of hello messages to users and applications.
``` 

This configuration file tells Phaka Service Wrapper that it should wrap `hello`. The service will be named `phaka-hello-service` and when users view it in the Services MMC plugin, should be shown as `Phaka Hello Service`, with the appropiate description.  
 
The Windows Service can be installed through several means:

- Windows Installer
- Command Line

To create the Windows Service, open a Command Prompt or PowerShell as an Administrator and run the following commands.

```
cd c:\tools
wrapper install
wrapper start
```

This changes the directory to `c:\tools` where `wrapper.exe` was installed. It then installs and starts the Windows Service. 

In order to see it function, look at `c:\tools\wrapper.log` and you'll see the output of `hello.exe`.

## Built With

* [Visual Studio 2017](https://www.visualstudio.com/downloads/)
* [Windows 10](https://www.microsoft.com/en-us/software-download/windows10) 

## Contributing

Please read [CONTRIBUTING.md](CONTRIBUTING.md) for details on our code of conduct, and the process for submitting pull requests to us.

## Versioning

We use [SemVer](http://semver.org/) for versioning. For the versions available, see the [tags on this repository](https://github.com/Phaka/Windows-Service-Wrapper/tags). 

## Authors

* [WernerStrydom](https://github.com/WernerStrydom) 

## License

This project is licensed under the MIT License - see the [LICENSE.md](LICENSE.md) file for details

## Acknowledgments

* [WinSW](https://github.com/kohsuke/winsw/)
