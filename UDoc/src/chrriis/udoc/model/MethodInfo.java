/*
 * Christopher Deckers (chrriis@nextencia.net)
 * http://www.nextencia.net
 * 
 * See the file "readme.txt" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
package chrriis.udoc.model;

public class MethodInfo {

  protected ClassInfo classInfo;
  protected String name;
  protected int modifiers;
  protected String[] genericDeclarations;
  protected FieldInfo[] parameters;
  protected FieldInfo returnedParameter;
  protected FieldInfo[] exceptionParameters;
  protected ClassInfo[] annotations;

  protected MethodInfo(ClassInfo classInfo, String name, int modifiers, String[] genericDeclarations) {
    this.classInfo = classInfo;
    this.name = name;
    this.modifiers = modifiers;
    this.genericDeclarations = genericDeclarations;
  }

  protected void setMethodParameters(FieldInfo[] parameters, FieldInfo returnedParameter, FieldInfo[] exceptionParameters) {
    this.parameters = parameters;
    this.returnedParameter = returnedParameter;
    this.exceptionParameters = exceptionParameters;
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
  
  public String[] getGenericDeclarations() {
    return genericDeclarations;
  }

  public String getName() {
    return name;
  }

  public int getModifiers() {
    return modifiers;
  }

  public FieldInfo[] getParameters() {
    if(parameters == null) {
      parameters = new FieldInfo[0];
    }
    return parameters;
  }

  public FieldInfo getReturnedParameter() {
    return returnedParameter;
  }

  public FieldInfo[] getExceptionParameters() {
    if(exceptionParameters == null) {
      exceptionParameters = new FieldInfo[0];
    }
    return exceptionParameters;
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
  
}
