using Microsoft.Tools.WindowsInstallerXml;
using System.Xml.Schema;
using System.Xml;
using System.Text.RegularExpressions;

namespace TextReplace
{
    public class TextReplaceExtension : WixExtension
    {
        Library _library;
        public override Library GetLibrary(TableDefinitionCollection tableDefinitions)
        {
            return _library ?? (_library = LoadLibraryHelper(System.Reflection.Assembly.GetExecutingAssembly(), "TextReplace.Resources.TextReplace.WixLib.wixlib", tableDefinitions));
        }

        public override TableDefinitionCollection TableDefinitions
        {
            get
            {
                return LoadTableDefinitionHelper(System.Reflection.Assembly.GetExecutingAssembly(), "TextReplace.Resources.tables.xml");
            }
        }

        public override CompilerExtension CompilerExtension
        {
            get
            {
                return new TextReplaceCompilerExtension();
            }
        }
    }

    public class TextReplaceCompilerExtension : CompilerExtension
    {
        XmlSchema _schema;

        public TextReplaceCompilerExtension()
        {
            _schema = XmlSchema.Read(System.Reflection.Assembly.GetExecutingAssembly().GetManifestResourceStream("TextReplace.Resources.TextReplace.xsd"), null);
        }

        public override XmlSchema Schema
        {
            get { return _schema; }
        }

        public override void ParseElement(SourceLineNumberCollection sourceLineNumbers, XmlElement parentElement, XmlElement element, params string[] contextValues)
        {
            string keyPath = null;
            ParseElement(sourceLineNumbers, parentElement, element, ref keyPath, contextValues);
        }

        public override ComponentKeypathType ParseElement(SourceLineNumberCollection sourceLineNumbers, XmlElement parentElement, XmlElement element, ref string keyPath, params string[] contextValues)
        {
            ComponentKeypathType keyType = ComponentKeypathType.None;
            switch (element.LocalName)
            {
                case "TextReplace":
                    switch (parentElement.LocalName)
                    {
                        case "ComponentRef":
                        case "Component":
                            ParseTextReplaceElement(sourceLineNumbers, parentElement, element);
                            break;
                        default:
                            Core.UnsupportedExtensionElement(parentElement, element);
                            break;
                    }
                    break;
                default:
                    Core.UnexpectedElement(parentElement, element);
                    break;
            }
            return keyType;
        }

        string SubstituteExpr(string str)
        {
            if (str.StartsWith("[@@]", System.StringComparison.Ordinal))
            { return str.Substring(4); }
            Regex r = new Regex(@"[[\]{}]");
            return r.Replace(str, @"[\$1]");
        }

        private void ParseTextReplaceElement(SourceLineNumberCollection sourceLineNumbers, XmlElement parentElement, XmlElement element)
        {
            string id = null;
            string matchExpr = null;
            string file = null;
            string component = null;
            string replaceExpr = null;
            component = parentElement.Attributes["Id"].Value;
            foreach (XmlAttribute attr in element.Attributes)
            {
                switch (attr.LocalName)
                {
                    case "Id":
                        id = attr.Value;
                        break;
                    case "Match":
                        matchExpr = attr.Value;
                        break;
                    case "File":
                        file = attr.Value;
                        break;
                    case "Replace":
                        replaceExpr = attr.Value;
                        break;
                    default:
                        Core.UnexpectedAttribute(sourceLineNumbers, attr);
                        break;
                }
            }
            if (id == null)
            { Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, element.Name, "Id")); }
            else if (file == null)
            { Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, element.Name, "File")); }
            else if (matchExpr == null)
            { Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, element.Name, "Match")); }
            else if (replaceExpr == null)
            { Core.OnMessage(WixErrors.ExpectedAttribute(sourceLineNumbers, element.Name, "Replace")); }
            else
            {
                Row r = Core.CreateRow(sourceLineNumbers, "TextReplace");
                r[0] = id;
                r[1] = component;
                r[2] = file;
                r[3] = SubstituteExpr(matchExpr);
                r[4] = SubstituteExpr(replaceExpr);
                Core.CreateWixSimpleReferenceRow(sourceLineNumbers, "CustomAction", "TextReplace");
            }
        }
    }
}
