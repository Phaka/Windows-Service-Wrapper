# Phaka Service Wrapper

The Phaka Service Wrapper (wrapper) runs another application, e.g. ASP.NET Core or a Java process, as a Windows service. It was written in C with the intent of having the least amount of requirements. The licensing also permits the repository to be forked, adapted and used in commercial products or otherwise which is useful when binaries needs to be signed by a commercial vendor when shipping it with their products.

The intent is to rename the library to something a bit more suited to the ultimate purpose. For example, if wrapper was wrapping the selenium hub process, then rename it to be `seleniumhub.exe`. If it wraps a ASP.NET Core application named `Hello`, then `hello.exe` or `hellosvc.exe` would be appropiate. If you rename the binary, remember to change the `wrapper`, `wrapper.exe`, `wrapper.cfg` in this document.

## Configuration

Phaka Service Wrapper needs a configuration file to work properly. When it starts either as a service or being ran as a user, it will determine the name of the executable, e.g. `c:\tools\wrapper.exe` and rename the extension to `.cfg` so that it treats `c:\tools\wrapper.cfg` as its configuration.  If you renamed `wrapper.exe` to `hello.exe`, then the configuration file will be `hello.cfg` and reside in the same directory as `hello.exe`.

Typically, the configuration file looks like the following. 

```
[Unit]
Name=service-name
CommandLine=sample1 arg1 arg2 arg3
Title=service title
Description=service description
```

The sections have the following meaning:

### Name 

The Name that will be registered in the Windows Service Control Manager (SCM). Once a the service was installed, this name should not change as it cause the Phaka Service Wrapper to no longer work as expected. If you'd like to rename a service, be sure to delete it first. 

Forward-slash (/) and backslash (\) are not valid service name characters. The name is limited to 256 characters. The name must be unique and you should not use an existing service name, especially that from  the operating system. This could result in the operating system becoming unstable. 

It is recommended to prefix the name with the organization, use all lowercase characters and seperate words with either an underscore or dash. For example, `phaka-selenium-hub`, `phaka-selenium-node` and `mycompany-hello-service` are all good names.

### Command Line

The command line will be executed by the Phaka Service Wrapper when the service starts.The first white space–delimited token of the command line specifies the module name that will be started. In the following case, it will run `sample1.exe`.

```
sample1 arg1 arg2 arg3
```

It will search for `sample1` as follows:

- The directory from which the wrapper loaded.
- The 32-bit Windows system directory. 
- The 16-bit Windows system directory. 
- The Windows directory. 
- The directories that are listed in the PATH environment variable. 

If the module name contains spaces, use quotation marks around the executable. For example, the following command line will attempt to run `program.exe` and not `sample1.exe`.

```
C:\Program Files\Phaka\bin\sample1.exe arg1 arg2 arg3
```

The correct mannger to specify this command line is 

```
"C:\Program Files\Phaka\bin\sample1.exe" arg1 arg2 arg3
```
#### See Also

- [CreateProcess](https://msdn.microsoft.com/en-us/library/windows/desktop/ms682425(v=vs.85).aspx)

### Title

The title or display name is the you'd like it to appear in Services. This isn't strictly required, but does make it much easier for humans to find the service. If the title is omitted, the `Name` property is used as the display name. Appropiate titles include `Phaka Hello Service` or `Hello`. 

If a title is specified, it must be less than 260 characters and may not only consist of spaces.

### Description 

The description property provides of the service. This is particulary useful for humans, asset management systems, and auditors. If the description property is empty, the service will be removed if one was specified before. Good examples of a description would be `Provides secure storage and retrieval of hello messages to users and applications.`.   

## Usage

The wrapper executable is intended to be used as a Windows Service or as a command line utility. Certain commands require that you run Command Prompt or PowerShell as an Administrator.  

```
wrapper [OPTIONS] COMMAND
```

### Options

The following options are understood:

- `--nologo`

    Suppresses the display of the banner and copyright information upon startup

### Commands

The following commands are understood:

#### help 
        
Display the usage message.

##### Example

```
wrapper help
```

#### install

Reads the name, title and command line from configuration file and then creates a service. If the service already exists, the command will fail and it will not update the existing service.

##### Example

```
wrapper install
```

#### delete

Reads the name from configuration file and then deletes the service with that name.

##### Example

```
wrapper delete
```

#### enable

Reads the name from configuration file and then enables the service with that name.

##### Example

```
wrapper enable
```

#### disable

Reads the name from configuration file and then disables the service with that name.

##### Example

```
wrapper disable
```

#### start

Reads the name from configuration file and then starts the service with that name. If the service is being stopped, it will wait for it to stop and then start it.

##### Example

```
wrapper start
```

#### stop

Reads the name from configuration file and then stops the service with that name. If the service is being started, it will wait for it to be in a running state and then stop it.

##### Example

```
wrapper stop
```

#### query

Reads the name from configuration file and then displays information about the service with that name. 

##### Example

```
wrapper query
```

#### update

Reads the name, title and description from configuration file and then updates the service accordinly. If the description is missing or empty, the existing description will be removed. if the title is missing, the name will be used. 

This command cannot be used to rename a service. In order to do that, you'll need to execute the delete command, update the configuration file and then execute the install command. 

##### Example

```
wrapper update
```

### Exit status

On success, 0 is returned, a non-zero failure code otherwise.

## Environment

There are no environment requirements.

## See Also

None.