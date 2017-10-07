# TextReplace Wix Extension

TextReplace is a WiX extension that allows you to do regular expression find and replace on text files during an install I built it because I had a particular need for something similar, and I had need of a new side project to keep me amused during the long commute to work.

## License
The code is licensed under the MIT License. See license.txt for details.

## Building

The component was designed to be build in VS2015. It may build in earlier versions, 
but this hasn't been tested. It will build in VS2015 Community. Note you will 
require both C# and C++ libraries installed in order to build it - the extension is
written in C#, but the custom action it's built on is written in C++. It just a case of loading the solution and hitting "build".

The built extension is under the TextReplace\bin folder. You only need the TextReplace.dll.

## Usage

To use text replace, you need to do two things:

1. Add the namespace `http://schemas.rottedfrog.co.uk/wix/TextReplace` to your WixElement:
2. Add `TextReplace` elements to the components you want to associate the change to.

The TextReplace element has 3 attributes:
| Attribute | Description                               |
|-----------|-------------------------------------------|
| Id        | A unique name to identify the replacement |
| File      | The name of a file. This field accepts formatted values |
| Match     | An Expression to replace with |
| Replace   | A replacement expression. You can use captured numbered references $1..$9 for different captures from the match expression. $& is the whole matched expression. |


#### Things to note about Match and replace:

The `Match` and `Replace` expressions use ECMAScript syntax for regexes and replacement strings. 
For more information go to
https://developer.mozilla.org/en/docs/Web/JavaScript/Guide/Regular_Expressions

Note that both expressions can be read either as formatted strings or non-formatted 
strings. They are by default not formatted. If you add the special prefix [@@], then the
expression will be read as a formatted string and you can include property values. Note 
that for Match, you will need to escape square brackets and braces if you make it 
formatted.

### Example:

````
<Wix xmlns="http://schemas.microsoft.com/wix/2006/wi" xmlns:tx="http://schemas.rottedfrog.co.uk/wix/TextReplace">
	<Fragment>
		<DirectoryRef Id="INSTALLFOLDER">
      <Component Id="Comp1">
        <File Id="testfile" Source="Testfile.txt" />
        <tx:TextReplace Id="text1" File="[#testfile]" Match="Hello " Replace="$&amp;$&amp;" />
      </Component>
		</DirectoryRef>
	</Fragment>
</Wix>
````

This will replace the string 'Hello World' in the test file with 'Hello Hello World'.
 
