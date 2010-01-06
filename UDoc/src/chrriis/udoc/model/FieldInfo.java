/*
 * Christopher Deckers (chrriis@nextencia.net)
 * http://www.nextencia.net
 * 
 * See the file "readme.txt" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
package chrriis.udoc.model;

import chrriis.udoc.model.processor.ClassProcessor;

public class FieldInfo {

  protected ClassInfo classInfo;
  protected MethodInfo methodInfo;
  protected String classDeclaration;
  protected String name;
  protected int modifiers;
  protected int[] classNamesIndices;
  protected ClassInfo[] classInfos;
  protected ClassInfo[] annotations;

  protected FieldInfo(ClassInfo classInfo, MethodInfo methodInfo, String classDeclaration, ClassProcessor classProcessor) {
    this(classInfo, methodInfo, classDeclaration, null, classProcessor);
  }

  protected FieldInfo(ClassInfo classInfo, MethodInfo methodInfo, String classDeclaration, String name, ClassProcessor classProcessor) {
    this(classInfo, methodInfo, classDeclaration, name, 0, classProcessor);
  }

  protected FieldInfo(ClassInfo classInfo, MethodInfo methodInfo, String classDeclaration, String name, int modifiers, ClassProcessor classProcessor) {
    this.classInfo = classInfo;
    this.methodInfo = methodInfo;
    this.name = name;
    this.classDeclaration = classDeclaration;
    this.modifiers = modifiers;
    classNamesIndices = getClassNameIndices(classDeclaration);
    classInfos = new ClassInfo[classNamesIndices.length / 2];
    for(int i=0; i<classInfos.length; i++) {
      classInfos[i] = ClassInfoLoader.createClassInfo(classDeclaration.substring(classNamesIndices[i * 2], classNamesIndices[i * 2 + 1]), classProcessor);
    }
  }

  public int getModifiers() {
    return modifiers;
  }

  public String getName() {
    return name;
  }

  public String getClassDeclaration() {
    return classDeclaration;
  }

  public int[] getClassNamesIndices() {
    return classNamesIndices;
  }

  public ClassInfo[] getClassInfos() {
    return classInfos;
  }

  protected int[] getClassNameIndices(String classDeclaration) {
    if(methodInfo == null) {
      return classInfo.getClassNameIndices(classDeclaration);
    }
    String[] methodGenerics = methodInfo.getGenericDeclarations();
    String[] classGenerics = classInfo.getGenericDeclarations();
    String[] generics = new String[methodGenerics.length + classGenerics.length];
    System.arraycopy(classGenerics, 0, generics, 0, classGenerics.length);
    System.arraycopy(methodGenerics, 0, generics, classGenerics.length, methodGenerics.length);
    return ClassInfo.getClassNameIndices(classDeclaration, generics);
//    return ClassInfo.getClassNameIndices("a.b.SomeClass<D extends a.b.Comparable<D> & a.b.Serializable,C>", generics);
  }

  public ClassInfo getClassInfo() {
    return classInfo;
  }

  protected String prototype;

  protected void setPrototype(String prototype) {
    this.prototype = prototype;
  }
  
  public String getPrototype() {
    return prototype;
  }
  
  protected void setAnnotations(ClassInfo[] annotations) {
    this.annotations = annotations;
    for(int i=0; i<annotations.length; i++) {
      if(annotations[i].getClassName().equals("java.lang.Deprecated")) {
        modifiers |= Modifiers.DEPRECATED;
        break;
      }
    }
  }
  
  public ClassInfo[] getAnnotations() {
    if(annotations == null) {
      return new ClassInfo[0];
    }
    return annotations;
  }
  
}
