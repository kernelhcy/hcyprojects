/*
 * Christopher Deckers (chrriis@nextencia.net)
 * http://www.nextencia.net
 * 
 * See the file "readme.txt" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
package chrriis.udoc.model.processor.binary;

import java.io.BufferedInputStream;
import java.io.ByteArrayInputStream;
import java.io.File;
import java.io.FileInputStream;
import java.io.InputStream;
import java.util.ArrayList;
import java.util.List;
import java.util.zip.ZipEntry;
import java.util.zip.ZipInputStream;

import javax.swing.JComponent;
import javax.swing.JTree;
import javax.swing.SwingUtilities;
import javax.swing.tree.DefaultMutableTreeNode;
import javax.swing.tree.DefaultTreeModel;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;

import org.objectweb.asm.AnnotationVisitor;
import org.objectweb.asm.ClassReader;
import org.objectweb.asm.FieldVisitor;
import org.objectweb.asm.MethodVisitor;
import org.objectweb.asm.Opcodes;
import org.objectweb.asm.Type;
import org.objectweb.asm.commons.EmptyVisitor;
import org.objectweb.asm.signature.SignatureReader;
import org.objectweb.asm.util.TraceSignatureVisitor;
import org.w3c.dom.Document;
import org.w3c.dom.Node;
import org.w3c.dom.NodeList;

import chrriis.udoc.model.Util;
import chrriis.udoc.model.processor.ClassProcessor;

public class BinaryClassProcessor extends ClassProcessor
{
	
	public String getProcessorID()
	{
		return "BinaryClassProcessor";
	}
	
	public String getProcessorName()
	{
		return "Binary";
	}
	
	public String getProcessorDescription()
	{
		return "Load class diagrams from \".class\" binary files";
	}
	
	protected String classPath;
	
	public JComponent getParametersComponent()
	{
		return new BinaryParametersComponent(this);
	}
	
	protected InputStream getClassInputStream(String className)
	{
		int lastDotIndex = className.lastIndexOf('.');
		String cName = className.substring(lastDotIndex + 1);
		String subPath = (lastDotIndex == -1 ? cName : className.substring(0, lastDotIndex + 1).replace('.', '/')
				+ cName)
				+ ".class";
		String[] paths = getClassPath().split(Util.getPathSeparator());
		for (int i = 0; i < paths.length; i++)
		{
			String path = paths[i];
			if (path.length() != 0)
			{
				File file = new File(path);
				if (file.exists())
				{
					if (file.isFile())
					{
						try
						{
							ZipInputStream zin = new ZipInputStream(new BufferedInputStream(new FileInputStream(file)));
							for (ZipEntry entry; (entry = zin.getNextEntry()) != null;)
							{
								if (subPath.equals(entry.getName()))
								{
									return zin;
								}
							}
						}
						catch (Exception e)
						{
							e.printStackTrace();
						}
					}
					else
					{
						try
						{
							return new BufferedInputStream(new FileInputStream(path + "/" + subPath));
						}
						catch (Exception e)
						{
							// e.printStackTrace();
						}
					}
				}
			}
		}
		return getClass().getResourceAsStream("/" + subPath);
	}
	
	public InputStream getClassInfoDataInputStream(String className)
	{
		int lastDotIndex = className.lastIndexOf('.');
		final String cName = Util.unescapeClassName(className.substring(lastDotIndex + 1));
		// String packageName = lastDotIndex == -1? "": className.substring(0,
		// lastDotIndex);
		InputStream in = getClassInputStream(className);
		if (in == null)
		{
			return null;
		}
		ClassReader classReader = null;
		try
		{
			classReader = new ClassReader(in);
		}
		catch (Exception e)
		{
			e.printStackTrace();
			return null;
		}
		StringBuffer classSB = new StringBuffer();
		classSB.append("<type name=\"").append(Util.escapeXML(className)).append("\">");
		final StringBuffer prototypeSB = new StringBuffer();
		final List constructorList = new ArrayList();
		final List superTypeList = new ArrayList();
		final List methodList = new ArrayList();
		final List fieldList = new ArrayList();
		classReader.accept(new EmptyVisitor()
		{
			protected int classAccess;
			protected StringBuffer lastSB;
			
			public void visit(int version, int access, String name, String signature, String superName,
					String[] interfaces)
			{
				access &= ~Opcodes.ACC_SYNCHRONIZED;
				classAccess = access;
				String eName = name.replace('/', '.');
				prototypeSB.append(getModifiers(access));
				boolean isClass = (access & (Opcodes.ACC_INTERFACE | Opcodes.ACC_ANNOTATION | Opcodes.ACC_ENUM)) == 0;
				if (isClass)
				{
					prototypeSB.append("class ");
				}
				prototypeSB.append(eName);
				if (signature != null)
				{
					TraceSignatureVisitor visitor = new TraceSignatureVisitor(access);
					new SignatureReader(signature).accept(visitor);
					String genericDeclaration = visitor.getDeclaration();
					if (genericDeclaration != null)
					{
						prototypeSB.append(genericDeclaration);
						int extendsIndex = -1;
						int implementsIndex = -1;
						int count = 0;
						for (int i = 0; i < genericDeclaration.length(); i++)
						{
							switch (genericDeclaration.charAt(i))
							{
								case '<':
									count++;
									break;
								case '>':
									count--;
									break;
								default:
									if (count == 0)
									{
										String substring = genericDeclaration.substring(i);
										if (extendsIndex == -1 && substring.startsWith(" extends "))
										{
											extendsIndex = i;
										}
										else if (implementsIndex == -1 && substring.startsWith(" implements "))
										{
											implementsIndex = i;
										}
									}
									break;
							}
						}
						// int index = extendsIndex > -1? extendsIndex:
						// implementsIndex;
						// if(index > -1) {
						// eName += genericDeclaration.substring(0,
						// index).trim();
						// } else {
						// eName += genericDeclaration;
						// }
						if (extendsIndex != -1)
						{
							addTypes(genericDeclaration.substring(extendsIndex + " extends ".length(),
									implementsIndex != -1 ? implementsIndex : genericDeclaration.length()),
									superTypeList, "class ");
						}
						else if (isClass)
						{
							superTypeList.add("public class java.lang.Object");
						}
						if (implementsIndex != -1)
						{
							addTypes(genericDeclaration.substring(implementsIndex + " implements ".length(),
									genericDeclaration.length()), superTypeList, "interface ");
						}
					}
				}
				else
				{
					// TODO: check if the superName is of the same type as the
					// current class, and add the modifier
					if ((access & Opcodes.ACC_INTERFACE) == 0 && superName != null)
					{
						String sName = superName.replace('/', '.');
						superTypeList.add("class " + sName);
						prototypeSB.append(" extends " + sName);
					}
					if (interfaces != null)
					{
						for (int i = 0; i < interfaces.length; i++)
						{
							String sName = interfaces[i].replace('/', '.');
							superTypeList.add("interface " + sName);
							if (i == 0)
							{
								if ((access & Opcodes.ACC_INTERFACE) == 0)
								{
									prototypeSB.append(" implements ");
								}
								else
								{
									prototypeSB.append(" extends ");
								}
							}
							else
							{
								prototypeSB.append(", ");
							}
							prototypeSB.append(sName);
						}
					}
				}
				lastSB = prototypeSB;
				super.visit(version, access, name, signature, superName, interfaces);
			}
			
			public AnnotationVisitor visitAnnotation(String desc, boolean visible)
			{
				if (lastSB != null)
				{
					lastSB.insert(0, '@' + getType(Type.getType(desc)) + ' ');
				}
				return super.visitAnnotation(desc, visible);
			}
			
			public FieldVisitor visitField(int access, String name, String desc, String signature, Object value)
			{
				if ((access & (Opcodes.ACC_SYNTHETIC | Opcodes.ACC_BRIDGE)) != 0)
				{
					lastSB = null;
					return super.visitField(access, name, desc, signature, value);
				}
				StringBuffer sb = new StringBuffer();
				sb.append(getModifiers(access));
				if (signature == null)
				{
					sb.append(getType(Type.getType(desc))).append(' ').append(name);
				}
				else
				{
					TraceSignatureVisitor visitor = new TraceSignatureVisitor(access);
					new SignatureReader(signature).accept(visitor);
					String returnType = visitor.getReturnType();
					if (returnType == null)
					{
						lastSB = null;
						return super.visitField(access, name, desc, signature, value);
					}
					sb.append(returnType).append(' ').append(name);
				}
				fieldList.add(sb);
				lastSB = sb;
				return super.visitField(access, name, desc, signature, value);
			}
			
			public MethodVisitor visitMethod(int access, String name, String desc, String signature, String[] exceptions)
			{
				if ("<clinit>".equals(name) || (access & Opcodes.ACC_SYNTHETIC) != 0)
				{
					lastSB = null;
					return super.visitMethod(access, name, desc, signature, exceptions);
				}
				StringBuffer sb = new StringBuffer();
				if ((classAccess & Opcodes.ACC_INTERFACE) != 0 && (classAccess & Opcodes.ACC_ANNOTATION) == 0)
				{
					access &= ~Opcodes.ACC_ABSTRACT;
				}
				sb.append(getModifiers(access));
				boolean isConstructor = "<init>".equals(name);
				if (isConstructor)
				{
					name = cName;
				}
				if (signature == null)
				{
					if (!isConstructor)
					{
						Type type = Type.getReturnType(desc);
						sb.append(getType(type)).append(' ');
					}
					sb.append(name).append('(');
					Type[] argumentTypes = Type.getArgumentTypes(desc);
					for (int i = 0; i < argumentTypes.length; i++)
					{
						if (i > 0)
						{
							sb.append(", ");
						}
						sb.append(getType(argumentTypes[i])).append(" arg" + i);
					}
					sb.append(')');
					if (exceptions != null)
					{
						sb.append(" throws ");
						for (int i = 0; i < exceptions.length; i++)
						{
							if (i > 0)
							{
								sb.append(", ");
							}
							sb.append(Util.escapeClassName(exceptions[i]));
						}
					}
				}
				else
				{
					TraceSignatureVisitor visitor = new TraceSignatureVisitor(access);
					new SignatureReader(signature).accept(visitor);
					String genericDeclaration = visitor.getDeclaration();
					if (genericDeclaration.charAt(0) != '(')
					{
						int index = genericDeclaration.indexOf('(');
						sb.append(genericDeclaration.substring(0, index)).append(' ');
						genericDeclaration = genericDeclaration.substring(index);
					}
					StringBuffer declarationSB = new StringBuffer(genericDeclaration.length() * 2);
					int count = 0;
					int iCount = 0;
					for (int i = 0; i < genericDeclaration.length(); i++)
					{
						char c = genericDeclaration.charAt(i);
						switch (c)
						{
							case '<':
								iCount++;
								declarationSB.append(c);
								break;
							case '>':
								iCount--;
								declarationSB.append(c);
								break;
							case ',':
								if (iCount != 0)
								{
									declarationSB.append(c);
									break;
								}
							case ')':
								if (i > 1)
								{
									declarationSB.append(" arg").append(count++);
								}
							default:
								declarationSB.append(c);
								break;
						}
					}
					String genericReturnType = visitor.getReturnType();
					String genericExceptions = visitor.getExceptions();
					if (!isConstructor)
					{
						sb.append(genericReturnType).append(' ');
					}
					sb.append(name).append(declarationSB.toString());
					if (genericExceptions != null)
					{
						sb.append(" throws ").append(genericExceptions);
					}
				}
				if (isConstructor)
				{
					constructorList.add(sb);
				}
				else
				{
					methodList.add(sb);
				}
				lastSB = sb;
				return super.visitMethod(access, name, desc, signature, exceptions);
			}
			
			public void visit(String name, Object value)
			{
				super.visit(name, value);
			}
			
			public void visitEnum(String name, String desc, String value)
			{
				if (lastSB != null)
				{
					int index = lastSB.indexOf(" ");
					if (index > 0)
					{
						if (name == null)
						{
							name = "value";
						}
						if (lastSB.charAt(index - 1) == ')')
						{
							lastSB.insert(index - 1, ", " + name + "=" + value);
						}
						else
						{
							lastSB.insert(index, "(" + name + "=" + value + ")");
						}
					}
				}
				super.visitEnum(name, desc, value);
			}
		}, ClassReader.SKIP_DEBUG);
		
		classSB.append("<prototype value=\"").append(Util.escapeXML(prototypeSB.toString())).append("\"/>");
		classSB.append("<superTypes>");
		for (int i = superTypeList.size() - 1; i >= 0; i--)
		{
			classSB.append("<superType value=\"").append(Util.escapeXML((String) superTypeList.get(i))).append("\"/>");
		}
		classSB.append("</superTypes>");
		classSB.append("<fields>");
		for (int i = fieldList.size() - 1; i >= 0; i--)
		{
			classSB.append("<field value=\"").append(Util.escapeXML(((StringBuffer) fieldList.get(i)).toString()))
					.append("\"/>");
		}
		classSB.append("</fields>");
		classSB.append("<constructors>");
		for (int i = constructorList.size() - 1; i >= 0; i--)
		{
			classSB.append("<constructor value=\"").append(
					Util.escapeXML(((StringBuffer) constructorList.get(i)).toString())).append("\"/>");
		}
		classSB.append("</constructors>");
		classSB.append("<methods>");
		for (int i = methodList.size() - 1; i >= 0; i--)
		{
			classSB.append("<method value=\"").append(Util.escapeXML(((StringBuffer) methodList.get(i)).toString()))
					.append("\"/>");
		}
		classSB.append("</methods>");
		classSB.append("</type>");
		System.err.println(classSB);
		try
		{
			return new ByteArrayInputStream(classSB.toString().getBytes("UTF-8"));
		}
		catch (Exception e)
		{
			e.printStackTrace();
		}
		return null;
	}
	
	protected static String getModifiers(int modifiers)
	{
		StringBuffer sb = new StringBuffer();
		if ((modifiers & Opcodes.ACC_PUBLIC) != 0)
		{
			sb.append("public ");
		}
		if ((modifiers & Opcodes.ACC_PROTECTED) != 0)
		{
			sb.append("protected ");
		}
		if ((modifiers & Opcodes.ACC_PRIVATE) != 0)
		{
			sb.append("private ");
		}
		if ((modifiers & Opcodes.ACC_NATIVE) != 0)
		{
			sb.append("native ");
		}
		if ((modifiers & Opcodes.ACC_STATIC) != 0)
		{
			sb.append("static ");
		}
		if ((modifiers & Opcodes.ACC_SYNCHRONIZED) != 0)
		{
			sb.append("synchronized ");
		}
		if ((modifiers & Opcodes.ACC_ENUM) != 0)
		{
			sb.append("enum ");
		}
		else
		{
			if ((modifiers & Opcodes.ACC_FINAL) != 0)
			{
				sb.append("final ");
			}
		}
		if ((modifiers & Opcodes.ACC_TRANSIENT) != 0)
		{
			sb.append("transient ");
		}
		if ((modifiers & Opcodes.ACC_VOLATILE) != 0)
		{
			sb.append("volatile ");
		}
		if ((modifiers & Opcodes.ACC_ANNOTATION) != 0)
		{
			sb.append("@interface ");
		}
		else if ((modifiers & Opcodes.ACC_INTERFACE) != 0)
		{
			sb.append("interface ");
		}
		else if ((modifiers & Opcodes.ACC_ABSTRACT) != 0)
		{
			sb.append("abstract ");
		}
		// if((modifiers & Opcodes.ACC_VARARGS) != 0) {
		// }
		// if((modifiers & Opcodes.ACC_BRIDGE) != 0) {
		// }
		// if((modifiers & Opcodes.ACC_DEPRECATED) != 0) {
		// }
		// if((modifiers & Opcodes.ACC_STRICT) != 0) {
		// }
		// if((modifiers & Opcodes.ACC_SUPER) != 0) {
		// }
		// if((modifiers & Opcodes.ACC_SYNTHETIC) != 0) {
		// }
		return sb.toString();
	}
	
	protected static String getType(Type type)
	{
		switch (type.getSort())
		{
			case Type.VOID:
				return "void";
			case Type.BOOLEAN:
				return "boolean";
			case Type.CHAR:
				return "char";
			case Type.BYTE:
				return "byte";
			case Type.SHORT:
				return "short";
			case Type.INT:
				return "int";
			case Type.FLOAT:
				return "float";
			case Type.LONG:
				return "long";
			case Type.DOUBLE:
				return "double";
			case Type.OBJECT:
				return type.getClassName();
			case Type.ARRAY:
			{
				StringBuffer sb = new StringBuffer();
				sb.append(getType(type.getElementType()));
				for (int i = type.getDimensions() - 1; i >= 0; i--)
				{
					sb.append("[]");
				}
				return sb.toString();
			}
		}
		return null;
	}
	
	protected void addTypes(String declaration, List list, String prefix)
	{
		StringBuffer sb = new StringBuffer();
		int count = 0;
		for (int i = 0; i < declaration.length(); i++)
		{
			char c = declaration.charAt(i);
			switch (c)
			{
				case '<':
					count++;
					sb.append(c);
					break;
				case '>':
					count--;
					sb.append(c);
					break;
				case ',':
					if (count == 0)
					{
						String type = sb.toString().trim();
						if (type.length() > 0)
						{
							list.add(prefix + type);
							sb = new StringBuffer();
						}
					}
					else
					{
						sb.append(c);
					}
					break;
				default:
					sb.append(c);
					break;
			}
		}
		String type = sb.toString().trim();
		if (type.length() > 0)
		{
			list.add(prefix + type);
		}
	}
	
	public String getClassPath()
	{
		return classPath == null ? "" : classPath;
	}
	
	public void setClassPath(String classPath)
	{
		this.classPath = classPath;
	}
	
	public String getXMLDescription()
	{
		StringBuffer sb = new StringBuffer();
		sb.append("<binaryClassProcessorParameter>");
		sb.append("<classPaths>");
		String[] classPaths = getClassPath().split(Util.getPathSeparator());
		for (int i = 0; i < classPaths.length; i++)
		{
			sb.append("<classPath value=\"").append(Util.escapeXML(classPaths[i])).append("\"/>");
		}
		sb.append("</classPaths>");
		sb.append("</binaryClassProcessorParameter>");
		return sb.toString();
	}
	
	public void loadXMLDescription(String xmlDescription)
	{
		try
		{
			DocumentBuilderFactory factory = DocumentBuilderFactory.newInstance();
			DocumentBuilder builder = factory.newDocumentBuilder();
			Document document = builder.parse(new ByteArrayInputStream(xmlDescription.getBytes("UTF-8")));
			NodeList nodeList = document.getChildNodes().item(0).getChildNodes();
			for (int i = 0; i < nodeList.getLength(); i++)
			{
				Node node = nodeList.item(i);
				String name = node.getNodeName();
				if ("classPaths".equals(name))
				{
					loadClassPaths(node.getChildNodes());
				}
			}
		}
		catch (Exception e)
		{
			e.printStackTrace();
		}
	}
	
	protected void loadClassPaths(NodeList nodeList)
	{
		classPath = null;
		StringBuffer sb = new StringBuffer();
		String pathSeparator = Util.getPathSeparator();
		for (int i = 0; i < nodeList.getLength(); i++)
		{
			Node node = nodeList.item(i);
			String name = node.getNodeName();
			if ("classPath".equals(name))
			{
				if (i > 0)
				{
					sb.append(pathSeparator);
				}
				sb.append(node.getAttributes().getNamedItem("value").getNodeValue());
			}
		}
		setClassPath(sb.toString());
	}
	
	protected DefaultMutableTreeNode javadocNode;
	
	public void loadClassBrowser(final JTree tree)
	{
		if (javadocNode != null)
		{
			tree.setModel(new DefaultTreeModel(javadocNode));
			return;
		}
		final String classPath = this.classPath;
		final DefaultMutableTreeNode node = (DefaultMutableTreeNode) tree.getModel().getRoot();
		node.removeAllChildren();
		new Thread()
		{
			public void run()
			{
				String[] paths = getClassPath().split(Util.getPathSeparator());
				for (int i = 0; i < paths.length; i++)
				{
					String path = paths[i];
					if (path.length() != 0)
					{
						File file = new File(path);
						if (file.exists())
						{
							if (file.isFile())
							{
								try
								{
									ZipInputStream zin = new ZipInputStream(new BufferedInputStream(
											new FileInputStream(file)));
									for (ZipEntry entry; (entry = zin.getNextEntry()) != null;)
									{
										String name = entry.getName();
										if (name.endsWith(".class"))
										{
											addTreeClass(name.substring(0, name.length() - ".class".length()).replace(
													'/', '.'), node);
										}
									}
								}
								catch (Exception e)
								{
									e.printStackTrace();
								}
							}
							else
							{
								addDirectoryContent(file, null, node);
							}
						}
					}
				}
				SwingUtilities.invokeLater(new Runnable()
				{
					public void run()
					{
						if (classPath == null ? BinaryClassProcessor.this.classPath != null : !classPath
								.equals(BinaryClassProcessor.this.classPath))
						{
							return;
						}
						javadocNode = node;
						tree.setModel(new DefaultTreeModel(javadocNode));
					}
				});
			}
		}.start();
	}
	
	protected static void addDirectoryContent(File directory, String currentPath, DefaultMutableTreeNode node)
	{
		File[] files = directory.listFiles();
		for (int i = 0; i < files.length; i++)
		{
			File file = files[i];
			String name = file.getName();
			if (file.isDirectory())
			{
				addDirectoryContent(file, currentPath == null ? name : currentPath + "." + name, node);
			}
			else if (name.endsWith(".class"))
			{
				name = name.substring(0, name.length() - ".class".length());
				addTreeClass(currentPath == null ? name : currentPath + "." + name, node);
			}
		}
	}
	
	protected static void addTreeClass(String className, DefaultMutableTreeNode node)
	{
		int index = className.indexOf('.');
		if (index != -1)
		{
			String packageName = className.substring(0, index);
			for (int i = node.getChildCount() - 1; i >= 0; i--)
			{
				DefaultMutableTreeNode childNode = (DefaultMutableTreeNode) node.getChildAt(i);
				String childPackageOrClassName = ((String) childNode.getUserObject());
				int comparison = childNode.isLeaf() ? 1 : childPackageOrClassName.compareTo(packageName);
				if (comparison == 0)
				{
					addTreeClass(className.substring(index + 1), childNode);
					return;
				}
				if (comparison < 0)
				{
					childNode = new DefaultMutableTreeNode(packageName);
					node.insert(childNode, i + 1);
					addTreeClass(className.substring(index + 1), childNode);
					return;
				}
			}
			DefaultMutableTreeNode childNode = new DefaultMutableTreeNode(packageName);
			node.insert(childNode, 0);
			addTreeClass(className.substring(index + 1), childNode);
			return;
		}
		for (int i = node.getChildCount() - 1; i >= 0; i--)
		{
			DefaultMutableTreeNode childNode = (DefaultMutableTreeNode) node.getChildAt(i);
			String childClassName = ((String) childNode.getUserObject());
			int comparison = !childNode.isLeaf() ? -1 : childClassName.compareTo(className);
			if (comparison == 0)
			{
				return;
			}
			if (comparison < 0)
			{
				childNode = new DefaultMutableTreeNode(Util.unescapeClassName(className));
				node.insert(childNode, i + 1);
				return;
			}
		}
		node.insert(new DefaultMutableTreeNode(Util.unescapeClassName(className)), 0);
	}
	
}
