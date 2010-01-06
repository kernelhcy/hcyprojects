/*
 * Christopher Deckers (chrriis@nextencia.net)
 * http://www.nextencia.net
 * 
 * See the file "readme.txt" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
package chrriis.udoc.ui;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Component;
import java.awt.Container;
import java.awt.Dimension;
import java.awt.FlowLayout;
import java.awt.Font;
import java.awt.FontMetrics;
import java.awt.Graphics;
import java.awt.Graphics2D;
import java.awt.LayoutManager;
import java.awt.Point;
import java.awt.Polygon;
import java.awt.Rectangle;
import java.awt.RenderingHints;
import java.awt.Toolkit;
import java.awt.event.AdjustmentEvent;
import java.awt.event.AdjustmentListener;
import java.awt.event.KeyAdapter;
import java.awt.event.KeyEvent;
import java.awt.event.MouseEvent;
import java.awt.event.MouseWheelEvent;
import java.awt.event.MouseWheelListener;
import java.awt.geom.AffineTransform;
import java.awt.geom.Rectangle2D;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.Comparator;
import java.util.HashMap;
import java.util.HashSet;
import java.util.Iterator;
import java.util.List;
import java.util.Locale;
import java.util.Map;
import java.util.Set;
import java.util.StringTokenizer;

import javax.swing.BorderFactory;
import javax.swing.JComponent;
import javax.swing.JPanel;
import javax.swing.JScrollBar;
import javax.swing.SwingUtilities;
import javax.swing.border.Border;
import javax.swing.event.MouseInputAdapter;

import chrriis.udoc.model.ClassInfo;
import chrriis.udoc.model.ClassInfoLoader;
import chrriis.udoc.model.Modifiers;
import chrriis.udoc.model.Util;
import chrriis.udoc.model.processor.ClassProcessor;
import chrriis.udoc.ui.layout.LayoutHandler;
import chrriis.udoc.ui.toolbar.ClassesToolBarMenu;
import chrriis.udoc.ui.toolbar.FilterToolBarMenu;
import chrriis.udoc.ui.toolbar.HelpToolBarMenu;
import chrriis.udoc.ui.toolbar.WorkspaceToolBarMenu;

public class ClassPane extends JPanel {

  protected static final Color SUPER_TYPE_FOCUSED_RELATION_COLOR = new Color(0, 12, 255);
  protected static final Color SUPER_TYPE_RELATION_COLOR = new Color(180, 184, 255);
  protected static final Color SUB_TYPE_FOCUSED_RELATION_COLOR = new Color(208, 79, 255);
  protected static final Color SUB_TYPE_RELATION_COLOR = new Color(236, 182, 255);
  protected static final Color COMPOSITION_FOCUSED_RELATION_COLOR = new Color(20, 147, 26);
  protected static final Color COMPOSITION_RELATION_COLOR = new Color(171, 217, 173);
  protected static final Color ASSOCIATION_FOCUSED_RELATION_COLOR = new Color(252, 116, 48);
  protected static final Color ASSOCIATION_RELATION_COLOR = new Color(253, 193, 162);
  
  protected static final Color SELECTION_COLOR = new Color(255, 0, 78);
  protected static final Color SELECTION_FILL_COLOR = new Color(SELECTION_COLOR.getRed(), SELECTION_COLOR.getGreen(), SELECTION_COLOR.getBlue(), 30);

  public static final int PUBLIC_VISIBILITY = 1;
  public static final int PROTECTED_VISIBILITY = 2;
  public static final int DEFAULT_VISIBILITY = 3;
  public static final int PRIVATE_VISIBILITY = 4;
  
  protected static final Border TOOL_BAR_BORDER = BorderFactory.createMatteBorder(0, 0, 1, 0, new Color(167, 166, 174));

  protected MouseHandler mouseHandler = new MouseHandler();

  class MouseHandler extends MouseInputAdapter {
    protected Point referenceLocation;
    protected boolean isControlPressed;
    protected boolean isShiftPressed;
    protected ClassComponent[] originalSelectedClassComponents;
    public void mousePressed(MouseEvent e) {
      closeMenus();
      if(e.getButton() != MouseEvent.BUTTON1) {
        return;
      }
      referenceLocation = e.getPoint();
      isControlPressed = (e.getModifiers() & MouseEvent.CTRL_MASK) != 0;
      isShiftPressed = (e.getModifiers() & MouseEvent.SHIFT_MASK) != 0;
      if(!isControlPressed) {
        clearSelectedClassComponents();
      }
      originalSelectedClassComponents = (ClassComponent[])selectedClassComponentSet.toArray(new ClassComponent[0]);
    }
    public void mouseReleased(MouseEvent e) {
      if(e.getButton() != MouseEvent.BUTTON1) {
        return;
      }
      selectionRectangle = null;
      referenceLocation = null;
      originalSelectedClassComponents = null;
      classComponentPane.repaint();
    }
    public void mouseDragged(MouseEvent e) {
      if(referenceLocation == null) {
        return;
      }
      Point location = e.getPoint();
      if(!isShiftPressed) {
        int offsetX = location.x - referenceLocation.x;
        int offsetY = location.y - referenceLocation.y;
        referenceLocation = location;
        moveClassComponents(offsetX, offsetY);
        return;
      }
      int x1 = Math.min(referenceLocation.x, location.x);
      int y1 = Math.min(referenceLocation.y, location.y);
      int x2 = Math.max(referenceLocation.x, location.x);
      int y2 = Math.max(referenceLocation.y, location.y);
      selectionRectangle = new Rectangle(x1, y1, x2 - x1, y2 - y1);
      clearSelectedClassComponents();
      if(isControlPressed) {
        for(int i=0; i<originalSelectedClassComponents.length; i++) {
          addSelectedClassComponent(originalSelectedClassComponents[i]);
        }
      }
      for(int i=classComponentPane.getComponentCount()-1; i>=0; i--) {
        ClassComponent classComponent = ((ClassComponent)classComponentPane.getComponent(i));
        Rectangle cBounds = classComponent.getBounds();
        cBounds.x--;
        cBounds.y--;
        cBounds.width++;
        cBounds.height++;
        if(cBounds.intersects(selectionRectangle)) {
          if(isControlPressed && isClassComponentSelected(classComponent)) {
            removeSelectedClassComponent(classComponent);
          } else {
            addSelectedClassComponent(classComponent);
          }
        }
      }
    }
  }

  protected Rectangle selectionRectangle;
  protected JPanel contentPane;
  protected JPanel menuPane;
  protected JPanel classComponentPane;
  protected JScrollBar vScrollBar;
  protected JScrollBar hScrollBar;
  protected JComponent toolBarComponent;

  public boolean isOptimizedDrawingEnabled() {
    return !menuPane.isVisible();
  }

  public ClassPane() {
    menuPane = new MenuPane();
    menuPane.setVisible(false);
    add(menuPane);
    contentPane = new JPanel(new BorderLayout(0, 0)) {
      public void reshape(int x, int y, int w, int h) {
        Rectangle cpBounds = getBounds();
        if(cpBounds.x == x && cpBounds.y == y && cpBounds.width == w && cpBounds.height == h) {
          return;
        }
        closeMenus();
        if(bounds != null) {
          Dimension size = getSize();
          int offsetX = 0;
          if(w > size.width) {
            offsetX = Math.max(0, Math.min(-hScrollBar.getMinimum(), w - size.width) - Math.max(0, bounds.x + bounds.width - w + (vScrollBar.isVisible()? vScrollBar.getWidth(): 0)));
          }
          int offsetY = 0;
          if(h > size.height) {
            offsetY = Math.max(0, Math.min(-vScrollBar.getMinimum(), h - size.height) - Math.max(0, bounds.y + bounds.height - h + (hScrollBar.isVisible()? hScrollBar.getHeight(): 0)));
          }
          moveClassComponents(offsetX, offsetY);
        }
        super.reshape(x, y, w, h);
        adjustBounds();
      }
    };
    classComponentPane = new JPanel(null) {
      public Color getBackground() {
        if(getComponentCount() == 0) {
          return new Color(230, 230, 240);
        }
        return super.getBackground();
      }
      protected void paintComponent(Graphics g) {
        super.paintComponent(g);
        String s = "Christopher Deckers - chrriis@nextencia.net";
        if(title != null) {
          s = title + " - " + s;
        }
        Font font = getFont().deriveFont(10.0f);
        g.setFont(font);
        FontMetrics fontMetrics = getFontMetrics(font);
        Rectangle2D bounds = fontMetrics.getStringBounds(s, g);
        Dimension size = contentPane.getSize();
        g.setColor(Color.gray);
        g.drawString(s, (int)Math.round(size.width - bounds.getWidth() - vScrollBar.getPreferredSize().width) - 3, fontMetrics.getAscent());
        paintClassComponentPane(g, false);
      }
      public void paint(Graphics g) {
        super.paint(g);
        if(selectionRectangle != null) {
          g.setColor(SELECTION_FILL_COLOR);
          g.fillRect(selectionRectangle.x, selectionRectangle.y, selectionRectangle.width, selectionRectangle.height);
          g.setColor(SELECTION_COLOR);
          g.drawRect(selectionRectangle.x, selectionRectangle.y, selectionRectangle.width, selectionRectangle.height);
        }
      }
      public void print(Graphics g) {
        paintClassComponentPane(g, true);
//        Rectangle bounds = getClassComponentPanePrintBounds();
//        g.translate(bounds.x, bounds.y);
        for(int i=getComponentCount()-1; i>=0; i--) {
          Component component = getComponent(i);
          if(component.isVisible()) {
            Point location = component.getLocation();
            g.translate(location.x, location.y);
            component.print(g);
            g.translate(-location.x, -location.y);
          }
        }
      }
      public boolean isOptimizedDrawingEnabled() {
        return !isOverlapping;
      }
    };
    classComponentPane.addMouseWheelListener(new MouseWheelListener() {
      public void mouseWheelMoved(MouseWheelEvent e) {
        int amount = e.getScrollType() == MouseWheelEvent.WHEEL_BLOCK_SCROLL? getScrollableBlockIncrement() * e.getScrollAmount(): e.getUnitsToScroll() * getScrollableUnitIncrement();
        moveClassComponents(0, -amount);
      }
    });
    classComponentPane.setBackground(Color.WHITE);
    contentPane.add(classComponentPane, BorderLayout.CENTER);
    classComponentPane.setOpaque(true);
    classComponentPane.addMouseListener(mouseHandler);
    classComponentPane.addMouseMotionListener(mouseHandler);
    classComponentPane.setFocusable(true);
    classComponentPane.addKeyListener(new KeyAdapter() {
      public void keyPressed(KeyEvent e) {
        boolean isControlPressed = (e.getModifiers() & KeyEvent.CTRL_MASK) != 0;
        boolean isShiftPressed = (e.getModifiers() & KeyEvent.SHIFT_MASK) != 0;
        switch(e.getKeyCode()) {
          case KeyEvent.VK_F5:
            if(isLocked()) {
              return;
            }
            if(isControlPressed && isShiftPressed) {
              setCrawling(!isCrawling());
            }
            ClassComponent[] classComponents = getSelectedClassComponents();
            if(classComponents.length == 0) {
              classComponents = getClassComponents();
            }
            final ClassComponent[] classComponents_ = classComponents;
            new Thread() {
              public void run() {
                for(int i=0; i<classComponents_.length; i++) {
                  final ClassComponent classComponent = classComponents_[i];
                  if(classComponent.isVisible()) {
                    classComponent.reload();
                    if(isCrawling()) {
                      SwingUtilities.invokeLater(new Runnable() {
                        public void run() {
                          classComponent.setRelationsVisible(true, Relation.SUPER_TYPE);
                          classComponent.setRelationsVisible(true, Relation.SUB_TYPE);
                          classComponent.setRelationsVisible(true, Relation.COMPOSITION);
                          classComponent.setRelationsVisible(true, Relation.ASSOCIATION);
                        }
                      });
                    }
                  }
                }
              }
            }.start();
            break;
          case KeyEvent.VK_DELETE:
            if(isLocked()) {
              return;
            }
            if(isShiftPressed) {
              deleteClassComponents(getSelectedClassComponents());
            } else {
              setClassComponentsFiltered(getSelectedClassComponents(), true);
            }
            break;
          case KeyEvent.VK_A:
            if(isControlPressed) {
              selectAllClassComponents();
            }
            break;
          case KeyEvent.VK_UP:
            if(selectedClassComponentSet.isEmpty()) {
              moveClassComponents(0, isControlPressed? -getScrollableBlockIncrement(): -getScrollableUnitIncrement());
            } else {
              moveSelectedClassComponents(0, isControlPressed? -getScrollableBlockIncrement(): -getScrollableUnitIncrement());
            }
            break;
          case KeyEvent.VK_DOWN:
            if(selectedClassComponentSet.isEmpty()) {
              moveClassComponents(0, isControlPressed? getScrollableBlockIncrement(): getScrollableUnitIncrement());
            } else {
              moveSelectedClassComponents(0, isControlPressed? getScrollableBlockIncrement(): getScrollableUnitIncrement());
            }
            break;
          case KeyEvent.VK_LEFT:
            if(selectedClassComponentSet.isEmpty()) {
              moveClassComponents(isControlPressed? -getScrollableBlockIncrement(): -getScrollableUnitIncrement(), 0);
            } else {
              moveSelectedClassComponents(isControlPressed? -getScrollableBlockIncrement(): -getScrollableUnitIncrement(), 0);
            }
            break;
          case KeyEvent.VK_RIGHT:
            if(selectedClassComponentSet.isEmpty()) {
              moveClassComponents(isControlPressed? getScrollableBlockIncrement(): getScrollableUnitIncrement(), 0);
            } else {
              moveSelectedClassComponents(isControlPressed? getScrollableBlockIncrement(): getScrollableUnitIncrement(), 0);
            }
            break;
          case KeyEvent.VK_L:
            if(isControlPressed) {
              adjustLayout();
            }
            break;
        }
      }
    });
    vScrollBar = new JScrollBar(JScrollBar.VERTICAL);
    contentPane.add(vScrollBar, BorderLayout.EAST);
    JPanel hScrollBarPanel = new JPanel(new BorderLayout(0, 0));
    hScrollBar = new JScrollBar(JScrollBar.HORIZONTAL);
    hScrollBarPanel.add(hScrollBar, BorderLayout.CENTER);
    hScrollBarPanel.add(new JPanel(null) {
      public Dimension getPreferredSize() {
        return vScrollBar.isVisible()? new Dimension(vScrollBar.getWidth(), 0): new Dimension(0, 0);
      }
    }, BorderLayout.EAST);
    contentPane.add(hScrollBarPanel, BorderLayout.SOUTH);
    adjustBounds();
    vScrollBar.addAdjustmentListener(new AdjustmentListener() {
      int diff;
      public void adjustmentValueChanged(AdjustmentEvent e) {
        isValueAdjusting = e.getValueIsAdjusting();
        if(!isValueAdjusting) {
          int value = diff - e.getValue();
          diff = 0;
          if(value != 0) {
            moveClassComponents(0, value);
          } else {
            adjustBounds();
            classComponentPane.repaint();
          }
        } else {
          if(diff != 0) {
            moveClassComponents(0, diff);
            diff = 0;
          }
          moveClassComponents(0, -e.getValue());
          diff -= -e.getValue();
        }
        isValueAdjusting = false;
      }
    });
    hScrollBar.addAdjustmentListener(new AdjustmentListener() {
      int diff;
      public void adjustmentValueChanged(AdjustmentEvent e) {
        isValueAdjusting = e.getValueIsAdjusting();
        if(!isValueAdjusting) {
          int value = diff - e.getValue();
          diff = 0;
          if(value != 0) {
            moveClassComponents(value, 0);
          } else {
            adjustBounds();
            classComponentPane.repaint();
          }
        } else {
          if(diff != 0) {
            moveClassComponents(diff, 0);
            diff = 0;
          }
          moveClassComponents(-e.getValue(), 0);
          diff -= -e.getValue();
        }
        isValueAdjusting = false;
      }
    });
    toolBarComponent = createToolBarComponent();
    contentPane.add(toolBarComponent, BorderLayout.NORTH);
    add(contentPane);
    setComponentZOrder(menuPane, 0);
    setLayout(createClassPaneLayout());
  }

  protected int getScrollableUnitIncrement() {
    return 10;
  }

  protected int getScrollableBlockIncrement() {
    return 50;
  }
  
  public void setContent(String classNames, ClassProcessor classProcessor) {
    classComponentPane.removeAll();
    relationList = new ArrayList();
    classInfoToClassComponentMap = new HashMap();
    final ClassComponent[] classComponents = addClasses(classNames, classProcessor);
    if(classComponents.length == 0) {
      return;
    }
    new Thread() {
      public void run() {
        for(int i=0; i<classComponents.length; i++) {
          final ClassComponent classComponent = classComponents[i];
          if(classComponent != null) {
            classComponent.load();
            SwingUtilities.invokeLater(new Runnable() {
              public void run() {
                classComponent.setExpanded(true);
                classComponent.setRelationsVisible(true, Relation.SUPER_TYPE);
                classComponent.setRelationsVisible(true, Relation.SUB_TYPE);
                classComponent.setRelationsVisible(true, Relation.COMPOSITION);
                classComponent.setRelationsVisible(true, Relation.ASSOCIATION);
                clearSelectedClassComponents();
              }
            });
          }
        }
      }
    }.start();
  }

  protected ClassProcessor lastClassProcessor;

  public ClassProcessor getLastClassProcessor() {
    return lastClassProcessor;
  }

  public void setLastClassProcessor(ClassProcessor lastClassProcessor) {
    this.lastClassProcessor = lastClassProcessor;
  }

  protected String lastClassNames;
  
  public String getLastClassNames() {
    return lastClassNames;
  }
  
  public void setLastClassNames(String lastClassNames) {
    this.lastClassNames = lastClassNames;
  }
  
  public ClassComponent[] addClasses(String classNames, ClassProcessor classProcessor) {
    if(classNames == null || classNames.length() == 0) {
      return new ClassComponent[0];
    }
    StringTokenizer st = new StringTokenizer(classNames, Util.getPathSeparator() + " ");
    int count = st.countTokens();
    if(count == 0) {
      return new ClassComponent[0];
    }
    boolean isComponentAdded = false;
    ClassComponent[] classComponents = new ClassComponent[count];
    lastClassNames = classNames;
    lastClassProcessor = classProcessor;
    List componentToPositionList = new ArrayList();
    for(int i=0; st.hasMoreTokens(); i++) {
      String className = st.nextToken();
      if(className.length() != 0) {
        ClassInfo classInfo = ClassInfoLoader.createClassInfo(className, classProcessor);
        boolean isClassComponentPresent = isClassComponentPresent(classInfo);
        ClassComponent classComponent = createClassComponent(classInfo);
        removeFilters(classInfo);
        if(!isClassComponentPresent) {
          addClassComponent(classComponent);
          isComponentAdded = true;
          componentToPositionList.add(classComponent);
        }
        classComponentPane.setComponentZOrder(classComponent, 0);
        classComponents[i] = classComponent;
      }
    }
    if(isComponentAdded) {
      int componentCount = componentToPositionList.size();
      int width = 0;
      int SPACE = 50;
      for(int i=0; i<componentCount; i++) {
        Dimension cSize = ((Component)componentToPositionList.get(i)).getSize();
        if(i > 0) {
          width += SPACE;
        }
        width += cSize.width;
      }
      Dimension size = classComponentPane.getSize();
      int start = (size.width - width) / 2;
      for(int i=0; i<componentCount; i++) {
        ClassComponent classComponent = (ClassComponent)componentToPositionList.get(i);
        Dimension cSize = classComponent.getSize();
        Point location = new Point(start, (size.height - cSize.height) / 2);
        classComponent.setLocation(location);
        classNameToLocationMap.put(classComponent.getClassInfo().getClassName(), location);
        start += cSize.width;
        start += SPACE;
      }
      
      adjustBounds();
    }
    return classComponents;
  }

  protected List relationList = new ArrayList();

  protected void setDevelopped(ClassComponent classComponent, boolean isDevelopped) {
    adjustBounds();
    classComponentPane.repaint();
  }

  protected void setRelationsVisible(ClassComponent classComponent, ClassInfo[] relatedClassInfos, boolean areRelationsVisible, final int relationType) {
    ArrayList relationList = new ArrayList();
    Set componentSet = new HashSet();
    for(int i=0; i<relatedClassInfos.length; i++) {
      ClassComponent relatedClassComponent = createClassComponent(relatedClassInfos[i]);
      relationList.add(new Relation(relationType, classComponent, relatedClassComponent));
      componentSet.add(relatedClassComponent);
    }
    if(!areRelationsVisible) {
      this.relationList.removeAll(relationList);
      for(int i=this.relationList.size()-1; i>=0; i--) {
        Relation relation = (Relation)this.relationList.get(i);
        componentSet.remove(relation.getClassComponent1());
        componentSet.remove(relation.getClassComponent2());
      }
      removeClassComponents((ClassComponent[])componentSet.toArray(new ClassComponent[0]));
      return;
    }
    this.relationList.addAll(relationList);
    for(Iterator it=componentSet.iterator(); it.hasNext(); ) {
      ClassComponent newClassComponent = (ClassComponent)it.next();
      if(newClassComponent.getParent() == null) {
        addClassComponent(newClassComponent);
      } else {
        it.remove();
      }
    }
    adjustClassComponents(classComponent, (ClassComponent[])componentSet.toArray(new ClassComponent[0]), relationType);
  }

  protected void adjustClassComponents(ClassComponent mainClassComponent, ClassComponent[] classComponents, final int relationType) {
    if(classComponents.length == 0) {
      return;
    }
    clearSelectedClassComponents();
    List componentList = new ArrayList();
    for(int i=0; i<classComponents.length; i++) {
      ClassComponent classComponent = classComponents[i];
      Point location = (Point)classNameToLocationMap.get(classComponent.getClassInfo().getClassName());
      if(location != null) {
        classComponent.setLocation(location);
        addSelectedClassComponent(classComponent);
      } else {
        componentList.add(classComponent);
      }
    }
    classComponents = (ClassComponent[])componentList.toArray(new ClassComponent[0]);
    Arrays.sort(classComponents, new Comparator() {
      public int compare(Object o1, Object o2) {
        ClassInfo c1 = ((ClassComponent)o1).getClassInfo();
        ClassInfo c2 = ((ClassComponent)o2).getClassInfo();
        int m1 = c1.getModifiers();
        int m2 = c2.getModifiers();
        int result = 0;
        if((m1 & Modifiers.INTERFACE) != 0) {
          if((m2 & Modifiers.CLASS) != 0) {
            result = 1;
          } else {
            result = c1.getName().toLowerCase(Locale.ENGLISH).compareTo(c2.getName().toLowerCase(Locale.ENGLISH));
          }
        } else if((m2 & Modifiers.INTERFACE) != 0) {
          result = -1;
        } else {
          result = c1.getName().toLowerCase(Locale.ENGLISH).compareTo(c2.getName().toLowerCase(Locale.ENGLISH));
        }
        if(relationType == Relation.SUB_TYPE || relationType == Relation.COMPOSITION) {
          result = -result;
        }
        return result;
      }
    });
    double angle = Math.PI / 2 / (classComponents.length + 1);
    Rectangle bounds = mainClassComponent.getBounds();
    Point origin = new Point(bounds.x + bounds.width / 2, bounds.y + bounds.height / 2);
    int distance = 30 * classComponents.length + 40 + (int)Math.round(Math.sqrt(Math.pow(origin.x - bounds.x, 2) + Math.pow(origin.y - bounds.y, 2)));
    for(int i=0; i<classComponents.length; i++) {
      ClassComponent classComponent = classComponents[i];
      Rectangle cBounds = classComponent.getBounds();
      AffineTransform at = new AffineTransform();
      double cAngle = angle * (i + 1);
      switch(relationType) {
        case Relation.SUPER_TYPE:
          cAngle -= Math.PI / 4;
          break;
        case Relation.SUB_TYPE:
          cAngle += 3 * Math.PI / 4;
          break;
        case Relation.COMPOSITION:
          cAngle -= 3 * Math.PI / 4;
          break;
        case Relation.ASSOCIATION:
          cAngle += Math.PI / 4;
          break;
      }
      at.setToRotation(cAngle, origin.x, origin.y);
      Point result = new Point();
      at.transform(new Point(origin.x, origin.y - distance), result);
      result.x -= cBounds.width / 2;
      result.y -= cBounds.height / 2;
      classNameToLocationMap.put(classComponent.getClassInfo().getClassName(), result);
      classComponent.setLocation(result);
    }
    if(relationType == Relation.SUB_TYPE || relationType == Relation.COMPOSITION) {
      for(int i=0; i<classComponents.length; i++) {
        ClassComponent classComponent = classComponents[i];
        classComponentPane.setComponentZOrder(classComponent, 0);
        addSelectedClassComponent(classComponent);
      }
    } else {
      for(int i=classComponents.length-1; i>=0; i--) {
        ClassComponent classComponent = classComponents[i];
        classComponentPane.setComponentZOrder(classComponent, 0);
        addSelectedClassComponent(classComponent);
      }
    }
    classComponentPane.revalidate();
    classComponentPane.repaint();
    adjustBounds();
  }

  protected boolean isCrawling;
  
  public boolean isCrawling() {
    return isCrawling;
  }
  
  public void setCrawling(boolean isCrawling) {
    this.isCrawling = isCrawling;
  }
  
  protected HashMap classInfoToClassComponentMap = new HashMap();

  protected void addClassComponent(final ClassComponent classComponent) {
    ClassInfo classInfo = classComponent.getClassInfo();
    if(!classInfoToClassComponentMap.containsValue(classComponent)) {
      classInfoToClassComponentMap.put(classInfo, classComponent);
      classNameToLocationMap.put(classInfo.getClassName(), classComponent.getLocation());
    }
    classComponentPane.add(classComponent);
    if(isFiltered(classInfo)) {
      classComponent.setVisible(false);
    }
    classComponentPane.revalidate();
    classComponentPane.repaint();
    if(isCrawling) {
      new Thread() {
        public void run() {
          classComponent.load();
          SwingUtilities.invokeLater(new Runnable() {
            public void run() {
              classComponent.setRelationsVisible(true, Relation.SUPER_TYPE);
              classComponent.setRelationsVisible(true, Relation.SUB_TYPE);
              classComponent.setRelationsVisible(true, Relation.COMPOSITION);
              classComponent.setRelationsVisible(true, Relation.ASSOCIATION);
            }
          });
        }
      }.start();
    }
  }

  protected void removeClassComponents(ClassComponent[] classComponents) {
    for(int i=0; i<classComponents.length; i++) {
      ClassComponent classComponent = classComponents[i];
      ClassInfo classInfo = classComponent.getClassInfo();
      classNameToLocationMap.remove(classInfo.getClassName());
      classInfoToClassComponentMap.remove(classInfo);
      classComponentPane.remove(classComponent);
      for(int j=relationList.size()-1; j>=0; j--) {
        Relation relation = (Relation)relationList.get(j);
        if(relation.getClassComponent1() == classComponent || relation.getClassComponent2() == classComponent) {
          relationList.remove(j);
        }
      }
      ClassInfoLoader.destroyClassInfo(classInfo);
    }
    adjustBounds();
    classComponentPane.revalidate();
    classComponentPane.repaint();
  }

  protected boolean isClassComponentPresent(ClassInfo classInfo) {
    return classInfoToClassComponentMap.get(classInfo) != null;
  }

  protected ClassComponent createClassComponent(ClassInfo classInfo) {
    ClassComponent classComponent = (ClassComponent)classInfoToClassComponentMap.get(classInfo);
    if(classComponent == null) {
      classComponent = new ClassComponent(this, classInfo);
      classInfoToClassComponentMap.put(classInfo, classComponent);
    }
    return classComponent;
  }

  public ClassComponent getClassComponent(ClassInfo classInfo) {
    return (ClassComponent)classInfoToClassComponentMap.get(classInfo);
  }
  
  protected ClassComponent getClassComponent(String className) {
    for(Iterator it=classInfoToClassComponentMap.values().iterator(); it.hasNext(); ) {
      ClassComponent classComponent = (ClassComponent)it.next();
      if(className.equals(classComponent.getClassInfo().getClassName())) {
        return classComponent;
      }
    }
    return null;
  }

  public Dimension getPreferredSize() {
    Dimension size = new Dimension();
    for(int i=classComponentPane.getComponentCount()-1; i>=0; i--) {
      Rectangle bounds = classComponentPane.getComponent(i).getBounds();
      size.width = Math.max(size.width, bounds.x + bounds.width);
      size.height = Math.max(size.height, bounds.y + bounds.height);
    }
    return size;
  }

  protected Collection filterRelationList() {
    Map hashMap = new HashMap();
    for(int i=relationList.size()-1; i>=0; i--) {
      Relation relation = (Relation)relationList.get(i);
      Object key = relation.getKey();
      Relation storedRelation = (Relation)hashMap.get(key);
      if(storedRelation != null) {
        if(storedRelation.getType() > relation.getType()) {
          hashMap.put(key, relation);
        }
      } else {
        hashMap.put(key, relation);
      }
    }
    if(hashMap.size() == relationList.size()) {
      return relationList;
    }
    return hashMap.values();
  }

  protected void paintClassComponentPane(Graphics g, boolean isPrinting) {
    Collection relationList = filterRelationList();
    for(Iterator it=relationList.iterator(); it.hasNext(); ) {
      Relation relation = (Relation)it.next();
      ClassComponent classComponent1 = relation.getClassComponent1();
      ClassComponent classComponent2 = relation.getClassComponent2();
      if(classComponent1.isVisible() && classComponent2.isVisible()) {
        Point center1 = classComponent1.getCenter();
        Point center2 = classComponent2.getCenter();
        Point p1 = classComponent1.getRelationPoint(center2);
        if(p1 != null) {
          Point p2 = classComponent2.getRelationPoint(center1);
          if(p2 != null) {
            switch(relation.getType()) {
              case Relation.SUPER_TYPE: {
                int length = (int)Math.round(Math.sqrt(Math.pow((p2.x - p1.x), 2) + Math.pow((p2.y - p1.y), 2)));
                int arrowLength = 12;
                if(length <= arrowLength + 2) {
                  if(isPrinting || isClassComponentSelected(classComponent1) || isClassComponentSelected(classComponent2)) {
                    g.setColor(SUPER_TYPE_FOCUSED_RELATION_COLOR);
                  } else {
                    g.setColor(SUPER_TYPE_RELATION_COLOR);
                  }
                  g.drawLine(p1.x, p1.y, p2.x, p2.y);
                  break;
                }
                int arrowBase = 5;
                int x = p1.x;
                int y = p1.y;
                Polygon polygon = new Polygon(new int[] {x, x, x - arrowBase, x, x + arrowBase, x}, new int[] {y, y - length + arrowLength, y - length + arrowLength, y - length, y - length + arrowLength, y - length + arrowLength}, 6);
                double angle = Math.atan(((double)(p2.x - p1.x)) / (p2.y - p1.y));
                if(p1.y <= p2.y) {
                  angle += Math.PI;
                }
                AffineTransform at = AffineTransform.getRotateInstance(-angle, p1.x, p1.y);
                g.setColor(Color.WHITE);
                ((Graphics2D)g).fill(at.createTransformedShape(polygon));
                if(isPrinting || isClassComponentSelected(classComponent1) || isClassComponentSelected(classComponent2)) {
                  g.setColor(SUPER_TYPE_FOCUSED_RELATION_COLOR);
                } else {
                  g.setColor(SUPER_TYPE_RELATION_COLOR);
                }
                ((Graphics2D)g).draw(at.createTransformedShape(polygon));
                break;
              }
              case Relation.SUB_TYPE: {
                int length = (int)Math.round(Math.sqrt(Math.pow((p1.x - p2.x), 2) + Math.pow((p1.y - p2.y), 2)));
                int arrowLength = 12;
                if(length <= arrowLength + 2) {
                  if(isPrinting || isClassComponentSelected(classComponent1) || isClassComponentSelected(classComponent2)) {
                    g.setColor(SUB_TYPE_FOCUSED_RELATION_COLOR);
                  } else {
                    g.setColor(SUB_TYPE_RELATION_COLOR);
                  }
                  g.drawLine(p1.x, p1.y, p2.x, p2.y);
                  break;
                }
                int arrowBase = 5;
                int x = p2.x;
                int y = p2.y;
                Polygon polygon = new Polygon(new int[] {x, x, x - arrowBase, x, x + arrowBase, x}, new int[] {y, y - length + arrowLength, y - length + arrowLength, y - length, y - length + arrowLength, y - length + arrowLength}, 6);
                double angle = Math.atan(((double)(p1.x - p2.x)) / (p1.y - p2.y));
                if(p2.y <= p1.y) {
                  angle += Math.PI;
                }
                AffineTransform at = AffineTransform.getRotateInstance(-angle, p2.x, p2.y);
                g.setColor(Color.WHITE);
                ((Graphics2D)g).fill(at.createTransformedShape(polygon));
                if(isPrinting || isClassComponentSelected(classComponent1) || isClassComponentSelected(classComponent2)) {
                  g.setColor(SUB_TYPE_FOCUSED_RELATION_COLOR);
                } else {
                  g.setColor(SUB_TYPE_RELATION_COLOR);
                }
                ((Graphics2D)g).draw(at.createTransformedShape(polygon));
                break;
              }
              case Relation.COMPOSITION: {
                int length = (int)Math.round(Math.sqrt(Math.pow((p1.x - p2.x), 2) + Math.pow((p1.y - p2.y), 2)));
                int diamondArrowLength = 8;
                if(length <= diamondArrowLength * 2 + 2) {
                  if(isPrinting || isClassComponentSelected(classComponent1) || isClassComponentSelected(classComponent2)) {
                    g.setColor(COMPOSITION_FOCUSED_RELATION_COLOR);
                  } else {
                    g.setColor(COMPOSITION_RELATION_COLOR);
                  }
                  g.drawLine(p1.x, p1.y, p2.x, p2.y);
                  break;
                }
                int arrowBase = 5;
                int x = p2.x;
                int y = p2.y;
                Polygon polygon = new Polygon(new int[] {x, x, x - arrowBase, x, x + arrowBase, x}, new int[] {y, y - length + diamondArrowLength * 2, y - length + diamondArrowLength, y - length, y - length + diamondArrowLength, y - length + diamondArrowLength * 2}, 6);
                double angle = Math.atan(((double)(p1.x - p2.x)) / (p1.y - p2.y));
                if(p2.y <= p1.y) {
                  angle += Math.PI;
                }
                AffineTransform at = AffineTransform.getRotateInstance(-angle, p2.x, p2.y);
                g.setColor(Color.WHITE);
                ((Graphics2D)g).fill(at.createTransformedShape(polygon));
                if(isPrinting || isClassComponentSelected(classComponent1) || isClassComponentSelected(classComponent2)) {
                  g.setColor(COMPOSITION_FOCUSED_RELATION_COLOR);
                } else {
                  g.setColor(COMPOSITION_RELATION_COLOR);
                }
                ((Graphics2D)g).draw(at.createTransformedShape(polygon));
                break;
              }
              case Relation.ASSOCIATION: {
                int length = (int)Math.round(Math.sqrt(Math.pow((p2.x - p1.x), 2) + Math.pow((p2.y - p1.y), 2)));
                int arrowLength = 8;
                if(length <= arrowLength + 2) {
                  if(isPrinting || isClassComponentSelected(classComponent1) || isClassComponentSelected(classComponent2)) {
                    g.setColor(ASSOCIATION_FOCUSED_RELATION_COLOR);
                  } else {
                    g.setColor(ASSOCIATION_RELATION_COLOR);
                  }
                  g.drawLine(p1.x, p1.y, p2.x, p2.y);
                  break;
                }
                int arrowBase = 4;
                int x = p1.x;
                int y = p1.y;
                Polygon polygon = new Polygon(new int[] {x, x, x - arrowBase, x, x + arrowBase, x}, new int[] {y, y - length, y - length + arrowLength, y - length, y - length + arrowLength, y - length}, 6);
                double angle = Math.atan(((double)(p2.x - p1.x)) / (p2.y - p1.y));
                if(p1.y <= p2.y) {
                  angle += Math.PI;
                }
                AffineTransform at = AffineTransform.getRotateInstance(-angle, p1.x, p1.y);
                g.setColor(Color.WHITE);
                ((Graphics2D)g).fill(at.createTransformedShape(polygon));
                if(isPrinting || isClassComponentSelected(classComponent1) || isClassComponentSelected(classComponent2)) {
                  g.setColor(ASSOCIATION_FOCUSED_RELATION_COLOR);
                } else {
                  g.setColor(ASSOCIATION_RELATION_COLOR);
                }
                ((Graphics2D)g).draw(at.createTransformedShape(polygon));
                break;
              }
            }
          }
        }
      }
    }
    if(classComponentPane.getComponentCount() == 0) {
      // Show some welcome message and basic help
      // The getClassComponentPanePrintBounds() method should be adjusted in case of a change here.
      int x = 40;
      int y = 45;
      g.drawImage(IconManager.getImage("UDoc64x64.png"), x, y, classComponentPane);
      Font font = new Font("SansSerif", Font.PLAIN, 20);
      g.setFont(font);
      g.setColor(Color.BLUE.darker());
      FontMetrics fm = Toolkit.getDefaultToolkit().getFontMetrics(font);
      int h = fm.getHeight();
      Object aaRenderingHint = ((Graphics2D)g).getRenderingHint(RenderingHints.KEY_ANTIALIASING);
      ((Graphics2D)g).setRenderingHint(RenderingHints.KEY_ANTIALIASING, RenderingHints.VALUE_ANTIALIAS_ON);
      g.drawString("Welcome to UDoc!", x + 64 + 10, y + (64 + h) / 2);
      ((Graphics2D)g).setRenderingHint(RenderingHints.KEY_ANTIALIASING, aaRenderingHint);
      y += 64;
      font = new Font("SansSerif", Font.PLAIN, 12);
      g.setFont(font);
      g.setColor(Color.BLUE);
      fm = Toolkit.getDefaultToolkit().getFontMetrics(font);
      h = fm.getHeight() + fm.getLeading();
      y += 20;
      g.drawString("To start, use one of the following arrow menus:", x, y + h);
      x += 20;
      y += h;
      g.drawString("- \"Workspace\" to load an existing workspace", x, y + h);
      y += h;
      g.drawString("- \"Classes\" to add some classes to this empty workspace", x, y + h);
    }
  }

  protected boolean isValueAdjusting;

  protected void moveClassComponents(int offsetX, int offsetY) {
    if(offsetX == 0 && offsetY == 0) {
      return;
    }
    closeMenus();
    for(int i=classComponentPane.getComponentCount()-1; i>=0; i--) {
      ClassComponent classComponent = (ClassComponent)classComponentPane.getComponent(i);
      Point location = classComponent.getLocation();
      location.x += offsetX;
      location.y += offsetY;
      classNameToLocationMap.put(classComponent.getClassInfo().getClassName(), location);
      classComponent.setLocation(location);
    }
    adjustBounds();
    classComponentPane.repaint();
  }

  protected void moveSelectedClassComponents(int offsetX, int offsetY) {
    moveSelectedClassComponent(null, offsetX, offsetY);
  }
  protected void moveSelectedClassComponent(ClassComponent mainClassComponent, int offsetX, int offsetY) {
    if(mainClassComponent != null) {
      Point p = mainClassComponent.getLocation();
      p.x += offsetX;
      p.y += offsetY;
      classNameToLocationMap.put(mainClassComponent.getClassInfo().getClassName(), p);
      mainClassComponent.setLocation(p);
    }
    for(Iterator it=selectedClassComponentSet.iterator(); it.hasNext(); ) {
      ClassComponent classComponent = (ClassComponent)it.next();
      if(classComponent != mainClassComponent) {
        Point location = classComponent.getLocation();
        location.x += offsetX;
        location.y += offsetY;
        classNameToLocationMap.put(classComponent.getClassInfo().getClassName(), location);
        classComponent.setLocation(location);
      }
    }
    adjustBounds();
    classComponentPane.repaint();
  }

  protected HashMap classNameToLocationMap = new HashMap();

  protected String[] getLocatedClasses() {
    return (String[])classNameToLocationMap.keySet().toArray(new String[0]);
  }

  protected Point getClassLocation(String className) {
    return (Point)classNameToLocationMap.get(className);
  }

  protected void clearClassLocations() {
    classNameToLocationMap.clear();
  }

  protected void setClassLocations(String[] classNames, Point[] locations) {
    clearClassLocations();
    for(int i=0; i<classNames.length; i++) {
      classNameToLocationMap.put(classNames[i], locations[i]);
    }
  }

  protected Rectangle bounds;
  protected boolean isOverlapping;
  
  public void adjustBounds() {
    if(isValueAdjusting) {
      return;
    }
    Dimension size = contentPane.getSize();
    Rectangle bounds = new Rectangle();
    isOverlapping = false;
    Rectangle visibleArea = new Rectangle(size);
    for(int i=classComponentPane.getComponentCount()-1; i>=0; i--) {
      Component classComponent = classComponentPane.getComponent(i);
      if(classComponent.isVisible()) {
        Rectangle cBounds = classComponent.getBounds();
        if(!isOverlapping && cBounds.intersects(visibleArea)) {
          for(int j=i-1; j>=0; j--) {
            Rectangle cBounds2 = classComponentPane.getComponent(j).getBounds();
            if(cBounds2.intersects(visibleArea) && cBounds2.intersects(cBounds)) {
              isOverlapping = true;
              break;
            }
          }
        }
        if(cBounds.x < bounds.x) {
          bounds.width += bounds.x - cBounds.x;
          bounds.x = cBounds.x;
        }
        bounds.width = Math.max(bounds.width, cBounds.x + cBounds.width - bounds.x + 1);
        if(cBounds.y < bounds.y) {
          bounds.height += bounds.y - cBounds.y;
          bounds.y = cBounds.y;
        }
        int toolBarHeight = toolBarComponent.isVisible()? toolBarComponent.getHeight(): 0;
        bounds.height = Math.max(bounds.height, cBounds.y + cBounds.height - bounds.y + toolBarHeight + 1);
      }
    }
    if(this.bounds == bounds) {
      return;
    }
    this.bounds = bounds;
    hScrollBar.setVisible(false);
    vScrollBar.setVisible(false);
    if(bounds.y < 0 || size.height < bounds.height) {
      vScrollBar.setVisible(true);
      size.width -= vScrollBar.getWidth();
    }
    if(bounds.x < 0 || size.width < bounds.width) {
      hScrollBar.setVisible(true);
      size.height -= hScrollBar.getHeight();
    }
    if(size.height < bounds.height && !vScrollBar.isVisible()) {
      vScrollBar.setVisible(true);
      size.width -= vScrollBar.getWidth();
    }
    hScrollBar.setValues(0, size.width, bounds.x, Math.max(size.width, bounds.x + bounds.width - 1));
    vScrollBar.setValues(0, size.height, bounds.y, Math.max(size.height, bounds.y + bounds.height - 1));
  }

  public Rectangle getClassComponentAreaBounds() {
    return bounds;
  }
  
  protected Set selectedClassComponentSet = new HashSet();

  protected boolean isClassComponentSelected(ClassComponent classComponent) {
    return selectedClassComponentSet.contains(classComponent);
  }

  public void addSelectedClassComponent(ClassComponent classComponent) {
    if(!classComponent.isVisible()) {
      return;
    }
    selectedClassComponentSet.add(classComponent);
    classComponent.setSelected(true);
    classComponentPane.repaint();
  }

  protected void removeSelectedClassComponent(ClassComponent classComponent) {
    selectedClassComponentSet.remove(classComponent);
    classComponent.setSelected(false);
    classComponentPane.repaint();
  }
  
  protected void setSelectedClassComponent(ClassComponent classComponent) {
    clearSelectedClassComponents();
    addSelectedClassComponent(classComponent);
  }

  protected void selectAllClassComponents() {
    for(int i=classComponentPane.getComponentCount()-1; i>=0; i--) {
      addSelectedClassComponent((ClassComponent)classComponentPane.getComponent(i));
    }
  }

  public ClassComponent[] getSelectedClassComponents() {
    return (ClassComponent[])selectedClassComponentSet.toArray(new ClassComponent[0]);
  }

  public void clearSelectedClassComponents() {
    for(Iterator it=selectedClassComponentSet.iterator(); it.hasNext(); ) {
      ((ClassComponent)it.next()).setSelected(false);
    }
    selectedClassComponentSet.clear();
    classComponentPane.repaint();
  }

  protected volatile boolean isAutoLoading;

  public boolean isAutoLoading() {
    return isAutoLoading;
  }

  public void setAutoLoading(boolean isAutoLoading) {
    this.isAutoLoading = isAutoLoading;
    if(!isAutoLoading) {
      return;
    }
    for(int i=classComponentPane.getComponentCount()-1; i>=0; i--) {
      ((ClassComponent)classComponentPane.getComponent(i)).autoLoad();
    }
  }

  public void setClassComponentsFiltered(ClassComponent[] classComponents, boolean areFiltered) {
    if(!areFiltered) {
      clearSelectedClassComponents();
    }
    for(int i=0; i<classComponents.length; i++) {
      ClassComponent classComponent = classComponents[i];
      ClassInfo classInfo = classComponent.getClassInfo();
      if(areFiltered) {
        addFilter(new Filter(classInfo.getClassName()));
        removeSelectedClassComponent(classComponent);
      } else {
        removeFilters(classInfo);
        addSelectedClassComponent(classComponent);
      }
    }
    classComponentPane.repaint();
  }

  protected boolean isFiltered(ClassInfo classInfo) {
    for(Iterator it=filterSet.iterator(); it.hasNext(); ) {
      Filter filter = (Filter)it.next();
      if(filter.matches(classInfo)) {
        return true;
      }
    }
    return false;
  }

  protected Set filterSet = new HashSet();

  public Filter[] getFilters() {
    return (Filter[])filterSet.toArray(new Filter[0]);
  }

  public void setFilters(Filter[] filters) {
    filterSet.clear();
    for(int i=0; i<filters.length; i++) {
      addFilter(filters[i]);
    }
  }

  public void addFilter(Filter filter) {
    filterSet.add(filter);
    for(int i=classComponentPane.getComponentCount()-1; i>=0; i--) {
      ClassComponent classComponent = (ClassComponent)classComponentPane.getComponent(i);
      if(filter.matches(classComponent.getClassInfo())) {
        classComponent.setVisible(false);
      }
    }
    classComponentPane.repaint();
  }

  public void removeFilter(Filter filter) {
    if(!filterSet.remove(filter)) {
      return;
    }
    Component[] components = classComponentPane.getComponents();
    for(int i=0; i<components.length; i++) {
      ClassComponent classComponent = (ClassComponent)components[i];
      if(filter.matches(classComponent.getClassInfo())) {
        classComponent.setVisible(true);
      }
    }
    classComponentPane.repaint();
  }

  protected void removeFilters(ClassInfo classInfo) {
    List nameFilterList = new ArrayList();
    Component[] components = classComponentPane.getComponents();
    for(Iterator it=filterSet.iterator(); it.hasNext(); ) {
      Filter filter = (Filter)it.next();
      if(filter.matches(classInfo)) {
        it.remove();
        for(int i=0; i<components.length; i++) {
          ClassComponent classComponent = (ClassComponent)components[i];
          ClassInfo cClassInfo = classComponent.getClassInfo();
          if(cClassInfo.equals(classInfo)) {
            classComponent.setVisible(true);
          } else {
            if(filter.matches(cClassInfo)) {
              nameFilterList.add(new Filter(cClassInfo.getClassName()));
            }
          }
        }
      }
    }
    for(int i=nameFilterList.size() - 1; i>=0; i--) {
      filterSet.add(nameFilterList.get(i));
    }
    classComponentPane.repaint();
  }

  public void deleteClassComponents(ClassComponent[] classComponents) {
    if(!areClassComponentsDeletable(classComponents)) {
      return;
    }
    removeClassComponents(classComponents);
  }

  public boolean areClassComponentsReloadable(ClassComponent[] classComponents) {
    if(classComponents.length == 0) {
      return false;
    }
    for(int i=0; i<classComponents.length; i++) {
      if(!classComponents[i].isReloadable()) {
        return false;
      }
    }
    return true;
  }
  
  public boolean areClassComponentsDeletable(ClassComponent[] classComponents) {
    if(classComponents.length == 0) {
      return false;
    }
    Set hashSet = new HashSet();
    for(int i=0; i<classComponents.length; i++) {
      ClassComponent classComponent = classComponents[i];
      hashSet.add(classComponent);
    }
    for(int i=relationList.size()-1; i>=0; i--) {
      Relation relation = (Relation)relationList.get(i);
      if(hashSet.contains(relation.getClassComponent1())) {
        if(!hashSet.contains(relation.getClassComponent2())) {
          return false;
        }
      } else if(hashSet.contains(relation.getClassComponent2())) {
        if(!hashSet.contains(relation.getClassComponent1())) {
          return false;
        }
      }
    }
    return true;
  }

  protected JComponent createToolBarComponent() {
    JPanel toolBarPanelComponent = new JPanel(new FlowLayout(FlowLayout.LEFT, 0, 0));
    toolBarPanelComponent.setBorder(TOOL_BAR_BORDER);
    toolBarPanelComponent.add(new WorkspaceToolBarMenu(this));
    toolBarPanelComponent.add(new ClassesToolBarMenu(this));
    toolBarPanelComponent.add(new FilterToolBarMenu(this));
    toolBarPanelComponent.add(new HelpToolBarMenu(this));
    return toolBarPanelComponent;
  }

  public ClassComponent[] getClassComponents() {
    Component[] components = classComponentPane.getComponents();
    ClassComponent[] classComponents = new ClassComponent[components.length];
    System.arraycopy(components, 0, classComponents, 0, components.length);
    return classComponents;
  }

  protected LayoutManager createClassPaneLayout() {
    return new LayoutManager() {
      public void addLayoutComponent(String name, Component comp) {}
      public void removeLayoutComponent(Component comp) {}
      public void layoutContainer(Container parent) {
        Dimension size = parent.getSize();
        contentPane.setSize(size);
        menuPane.setSize(size);
      }
      public Dimension minimumLayoutSize(Container parent) {
        return bounds.getSize();
      }
      public Dimension preferredLayoutSize(Container parent) {
        return bounds.getSize();
      }
    };
  }

  public JComponent getMenuPane() {
    return menuPane;
  }

  protected class MenuPane extends JPanel {
    public MenuPane() {
      super(null);
      setOpaque(false);
    }
    public void hide() {
      super.hide();
      for(int i=getComponentCount()-1; i>=0; i--) {
        Component component = getComponent(i);
        remove(i);
        component.setVisible(false);
      }
      if(classComponentPane != null) {
        classComponentPane.requestFocus();
      }
    }
    public boolean contains(int x, int y) {
      for(int i=getComponentCount()-1; i>=0; i--) {
        Component component = getComponent(i);
        if(component.getBounds().contains(x, y)) {
          return true;
        }
      }
      return false;
    }
    public void remove(Component comp) {
      super.remove(comp);
      comp.setVisible(false);
    }
    public boolean isOptimizedDrawingEnabled() {
      return getComponentCount() <= 1;
    }
  }

  protected JComponent toolTipComponent;

  public void showTooltip(JComponent tooltip, JComponent invoker, int x, int y) {
    hideTooltip();
    Point p = SwingUtilities.convertPoint(invoker, x, y, menuPane);
    // TODO: adjust tooltip to fit in window
    Dimension mSize = tooltip.getPreferredSize();
    Dimension size = getSize();
    if(p.x + mSize.width > size.width) {
      p.x = size.width - mSize.width;
    }
    if(p.x < 0) {
      p.x = 0;
    }
    if(p.y + mSize.height > size.height) {
      p.y = size.height - mSize.height;
    }
    if(p.y < 0) {
      p.y = 0;
    }
    tooltip.setLocation(p.x, p.y);
    toolTipComponent = tooltip;
    menuPane.add(tooltip);
    menuPane.revalidate();
    menuPane.repaint();
    menuPane.setVisible(true);
    showMenu(tooltip);
  }

  protected void showMenu(JComponent menu) {
    final Dimension size = menu.getPreferredSize();
    menu.setSize(new Dimension(size.width, 0));
    developMenu(menu, System.currentTimeMillis(), size);
  }
  
  protected void developMenu(final JComponent menuComponent, final long timeReference, final Dimension size) {
    Dimension d = new Dimension(size);
    float ratio = ((float)(System.currentTimeMillis() - timeReference)) / 100;
//    d.width = Math.round(d.width * ratio);
    d.height = Math.round(d.height * ratio);
    menuComponent.setSize(Math.min(size.width, d.width), Math.min(size.height, d.height));
    menuComponent.revalidate();
    menuComponent.repaint();
    if(size.height > d.height) {
      SwingUtilities.invokeLater(new Runnable() {
        public void run() {
          developMenu(menuComponent, timeReference, size);
        }
      });
    }
  }
  
  public void hideTooltip() {
    if(toolTipComponent == null) {
      return;
    }
    menuPane.remove(toolTipComponent);
    menuPane.setVisible(menuPane.getComponentCount() > 0);
    menuPane.revalidate();
    menuPane.repaint();
    toolTipComponent = null;
  }

  public void showMenu(JComponent component, Component invoker, int x, int y) {
    Point p = SwingUtilities.convertPoint(invoker, x, y, menuPane);
    component.setLocation(p.x, p.y);
    menuPane.add(component);
    menuPane.revalidate();
    menuPane.repaint();
    menuPane.setVisible(true);
    showMenu(component);
    hideTooltip();
  }

  public void closeMenus() {
    for(int i=menuPane.getComponentCount()-1; i>=0; i--) {
      Component c = menuPane.getComponent(i);
      if(c != toolTipComponent) {
        menuPane.remove(c);
      }
    }
    menuPane.setVisible(toolTipComponent != null);
    menuPane.revalidate();
    menuPane.repaint();
  }

  public Rectangle getClassComponentPanePrintBounds() {
    Rectangle bounds = null;
    int componentCount = classComponentPane.getComponentCount();
    if(componentCount == 0) {
      return new Rectangle(30, 30, 400, 200);
    }
    for(int i=componentCount-1; i>=0; i--) {
      Component classComponent = classComponentPane.getComponent(i);
      if(classComponent.isVisible()) {
        Rectangle cBounds = ((ClassComponent)classComponent).getBounds();
        if(bounds == null) {
          bounds = cBounds;
        } else {
          if(cBounds.x < bounds.x) {
            bounds.width += bounds.x - cBounds.x;
            bounds.x = cBounds.x;
          }
          bounds.width = Math.max(bounds.width, cBounds.x + cBounds.width - bounds.x);
          if(cBounds.y < bounds.y) {
            bounds.height += bounds.y - cBounds.y;
            bounds.y = cBounds.y;
          }
          bounds.height = Math.max(bounds.height, cBounds.y + cBounds.height - bounds.y);
        }
      }
    }
    return bounds;
  }

  public Container getClassComponentPane() {
    return classComponentPane;
  }

  protected int fieldVisibility = PROTECTED_VISIBILITY;
  protected int methodVisibility = PROTECTED_VISIBILITY;
  
  public int getFieldVisibility() {
    return fieldVisibility;
  }
  
  public int getMethodVisibility() {
    return methodVisibility;
  }
  
  public void setFieldVisibility(int fieldVisibility) {
    this.fieldVisibility = fieldVisibility;
    Component[] components = classComponentPane.getComponents();
    for(int i=0; i<components.length; i++) {
      ClassComponent classComponent = (ClassComponent)components[i];
      classComponent.updateFieldVisibility();
    }    
  }
  
  public void setMethodVisibility(int methodVisibility) {
    this.methodVisibility = methodVisibility;
    Component[] components = classComponentPane.getComponents();
    for(int i=0; i<components.length; i++) {
      ClassComponent classComponent = (ClassComponent)components[i];
      classComponent.updateMethodVisibility();
    }    
  }
  
  protected boolean isLocked;
  
  public boolean isLocked() {
    return isLocked;
  }
  
  public void setLocked(boolean isLocked) {
    if(this.isLocked == isLocked) {
      return;
    }
    this.isLocked = isLocked;
    toolBarComponent.setVisible(!isLocked);
    adjustBounds();
  }
  
  public void adjustLayout() {
    LayoutHandler.adjust(ClassPane.this);
  }
  
  protected String title;
  
  public void setTitle(String title) {
    if("".equals(title)) {
      title = null;
    }
    this.title = title;
  }
  
}
