/*
 * Christopher Deckers (chrriis@nextencia.net)
 * http://www.nextencia.net
 * 
 * See the file "readme.txt" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
package chrriis.udoc.model;

public interface Modifiers {

  public static final int PUBLIC = 1 << 0;
  public static final int PROTECTED = 1 << 1;
  public static final int DEFAULT = 1 << 2;
  public static final int PRIVATE = 1 << 3;
  public static final int ANNOTATION = 1 << 4;
  public static final int ENUM = 1 << 5;
  public static final int INTERFACE = 1 << 6;
  public static final int ABSTRACT = 1 << 7;
  public static final int CLASS = 1 << 8;
  public static final int STATIC = 1 << 9;
  public static final int FINAL = 1 << 10;
  public static final int VOLATILE = 1 << 11;
  public static final int NATIVE = 1 << 12;
  public static final int TRANSIENT = 1 << 13;
  public static final int SYNCHRONIZED = 1 << 14;
  public static final int FIELD = 1 << 15;
  public static final int METHOD = 1 << 16;
  public static final int CONSTRUCTOR = 1 << 17;
  public static final int NOT_LOADED = 1 << 18;
  public static final int LOADING = 1 << 19;
  public static final int LOADING_FAILED = 1 << 20;
  public static final int DEPRECATED = 1 << 21;

}
