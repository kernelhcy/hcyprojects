/*
 * Christopher Deckers (chrriis@nextencia.net)
 * http://www.nextencia.net
 * 
 * See the file "readme.txt" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
package chrriis.udoc.ui.toolbar;

import java.awt.BorderLayout;
import java.awt.Color;
import java.awt.Component;
import java.awt.Dimension;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.ItemEvent;
import java.awt.event.ItemListener;
import java.util.Arrays;
import java.util.Comparator;
import java.util.Locale;

import javax.swing.BorderFactory;
import javax.swing.BoxLayout;
import javax.swing.DefaultListCellRenderer;
import javax.swing.DefaultListModel;
import javax.swing.JCheckBox;
import javax.swing.JComboBox;
import javax.swing.JLabel;
import javax.swing.JList;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextField;
import javax.swing.ListCellRenderer;
import javax.swing.event.DocumentEvent;
import javax.swing.event.DocumentListener;
import javax.swing.event.ListSelectionEvent;
import javax.swing.event.ListSelectionListener;

import chrriis.udoc.model.Modifiers;
import chrriis.udoc.ui.ClassPane;
import chrriis.udoc.ui.Filter;
import chrriis.udoc.ui.IconManager;
import chrriis.udoc.ui.widgets.ArrowButton;

public class FilterToolBarMenu extends ToolBarMenu {

  protected static final Color SELECTION_COLOR = new Color(218, 219, 255);

  protected ClassPane classPane;

  public FilterToolBarMenu(ClassPane classPane) {
    super(IconManager.getIcon("tool_bar_on.gif"), IconManager.getIcon("tool_bar_off.gif"), classPane);
    this.classPane = classPane;
  }

  protected String getContentTitle() {
    return "Filters";
  }

  protected Color getSelectionColor() {
    return SELECTION_COLOR;
  }

  protected Component createPopupMenuContent() {
    final JList list = new JList(new DefaultListModel());
    list.setCellRenderer(new DefaultListCellRenderer() {
      public Component getListCellRendererComponent(JList list, Object value, int index, boolean isSelected, boolean cellHasFocus) {
        if(!(value instanceof Filter)) {
          return super.getListCellRendererComponent(list, value, index, isSelected, cellHasFocus);
        }
        Filter filter = (Filter)value;
        DefaultListCellRenderer renderer = (DefaultListCellRenderer)super.getListCellRendererComponent(list, getFilterDisplay(filter), index, isSelected, cellHasFocus);
        return renderer;
      }
    });
    JScrollPane scrollPane = new JScrollPane(list);
    scrollPane.setPreferredSize(new Dimension(200, 0));
    JPanel westPanel = new JPanel(new BorderLayout(0, 0));
    JPanel additionPanel = new JPanel();
    additionPanel.setBorder(BorderFactory.createTitledBorder("New Filter"));
    BoxLayout boxLayout = new BoxLayout(additionPanel, BoxLayout.Y_AXIS);
    additionPanel.setLayout(boxLayout);
    final JTextField expressionField = new JTextField(7);
    additionPanel.add(expressionField);
    final JCheckBox regularExpressionCheckBox = new JCheckBox("Regular Expression");
    additionPanel.add(regularExpressionCheckBox);
    final JCheckBox classCheckBox = new JCheckBox("Class");
    classCheckBox.setSelected(true);
    additionPanel.add(classCheckBox);
    final JCheckBox interfaceCheckBox = new JCheckBox("Interface");
    interfaceCheckBox.setSelected(true);
    additionPanel.add(interfaceCheckBox);
    final JCheckBox enumCheckBox = new JCheckBox("Enum");
    enumCheckBox.setSelected(true);
    additionPanel.add(enumCheckBox);
    final JCheckBox annotationCheckBox = new JCheckBox("Annotation");
    annotationCheckBox.setSelected(true);
    additionPanel.add(annotationCheckBox);
    ActionListener actionListener = new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        String pattern = expressionField.getText();
        if(pattern.length() == 0) {
          return;
        }
        boolean isClass = classCheckBox.isSelected();
        boolean isInterface = interfaceCheckBox.isSelected();
        boolean isEnum = enumCheckBox.isSelected();
        boolean isAnnotation = annotationCheckBox.isSelected();
        int modifiers = 0;
        if(isClass != isInterface || isClass != isEnum || isClass != isAnnotation) {
          if(isClass) {
            modifiers |= Modifiers.CLASS;
          }
          if(isInterface){
            modifiers |= Modifiers.INTERFACE;
          }
          if(isEnum){
            modifiers |= Modifiers.ENUM;
          }
          if(isAnnotation){
            modifiers |= Modifiers.ANNOTATION;
          }
        }
        boolean isRegularExpression = regularExpressionCheckBox.isSelected();
        try {
          Filter filter = new Filter(pattern, isRegularExpression, modifiers);
          expressionField.setText("");
          classPane.addFilter(filter);
          updateListModel(list);
        } catch(Exception ex) {
        }
      }
    };
    expressionField.addActionListener(actionListener);
    westPanel.add(additionPanel, BorderLayout.WEST);
    GridBagLayout gridBag = new GridBagLayout();
    JPanel centerPanel = new JPanel(gridBag);
    GridBagConstraints cons = new GridBagConstraints();
    cons.gridx = 0;
    cons.gridy = 0;
    cons.insets = new Insets(2, 2, 2, 2);
    final ArrowButton rightButton = new ArrowButton(ArrowButton.EAST);
    gridBag.setConstraints(rightButton, cons);
    centerPanel.add(rightButton);
    rightButton.setEnabled(false);
    rightButton.addActionListener(actionListener);
    expressionField.getDocument().addDocumentListener(new DocumentListener() {
      public void changedUpdate(DocumentEvent e) {
        rightButton.setEnabled(expressionField.getText().length() > 0);
      }
      public void insertUpdate(DocumentEvent e) {
        rightButton.setEnabled(expressionField.getText().length() > 0);
      }
      public void removeUpdate(DocumentEvent e) {
        rightButton.setEnabled(expressionField.getText().length() > 0);
      }
    });
    cons.gridy++;
    final ArrowButton leftButton = new ArrowButton(ArrowButton.WEST);
    gridBag.setConstraints(leftButton, cons);
    centerPanel.add(leftButton);
    leftButton.setEnabled(false);
    list.addListSelectionListener(new ListSelectionListener() {
      public void valueChanged(ListSelectionEvent e) {
        leftButton.setEnabled(list.getSelectedIndices().length > 0);
      }
    });
    leftButton.addActionListener(new ActionListener() {
      public void actionPerformed(ActionEvent e) {
        int index = list.getLeadSelectionIndex();
        Filter filter = null;
        if(index != -1) {
          filter = (Filter)list.getModel().getElementAt(index);
        }
        Object[] values = list.getSelectedValues();
        for(int i=0; i<values.length; i++) {
          classPane.removeFilter((Filter)values[i]);
        }
        updateListModel(list);
        if(filter != null) {
          expressionField.setText(filter.getNamePattern());
          regularExpressionCheckBox.setSelected(filter.isRegularExpression());
          int modifiers = filter.getModifiers();
          classCheckBox.setSelected((modifiers & Modifiers.CLASS) != 0);
          interfaceCheckBox.setSelected((modifiers & Modifiers.INTERFACE) != 0);
          enumCheckBox.setSelected((modifiers & Modifiers.ENUM) != 0);
          annotationCheckBox.setSelected((modifiers & Modifiers.ANNOTATION) != 0);
        }
      }
    });
    JPanel filterPane = new JPanel(new BorderLayout(0, 0)) {
      public void requestFocus() {
        expressionField.requestFocus();
      }
    };
    JPanel listPanel = new JPanel(new BorderLayout(0, 0));
    listPanel.add(scrollPane, BorderLayout.CENTER);
    listPanel.setBorder(BorderFactory.createTitledBorder("Current Filters"));
    filterPane.add(listPanel, BorderLayout.EAST);
    filterPane.add(centerPanel, BorderLayout.CENTER);
    filterPane.add(westPanel, BorderLayout.WEST);
    gridBag = new GridBagLayout();
    cons = new GridBagConstraints();
    JPanel visibilityPanel = new JPanel(gridBag);
    visibilityPanel.setBorder(BorderFactory.createTitledBorder("Least Visible Access"));
    cons.insets = new Insets(2, 2, 2, 2);
    cons.gridx = 0;
    cons.gridy = 0;
    cons.fill = GridBagConstraints.HORIZONTAL;
    JLabel fieldVisibilityLabel = new JLabel("Fields: ");
    gridBag.setConstraints(fieldVisibilityLabel, cons);
    visibilityPanel.add(fieldVisibilityLabel);
    cons.gridy++;
    JLabel methodVisibilityLabel = new JLabel("Methods: ");
    gridBag.setConstraints(methodVisibilityLabel, cons);
    visibilityPanel.add(methodVisibilityLabel);
    cons.gridx++;
    cons.gridy = 0;
    ListCellRenderer visibilityRenderer = new DefaultListCellRenderer() {
      public Component getListCellRendererComponent(JList list, Object value, int index, boolean isSelected, boolean cellHasFocus) {
        if(value instanceof Integer) {
          switch(((Integer)value).intValue()) {
          case ClassPane.PUBLIC_VISIBILITY:
            value = "Public";
            break;
          case ClassPane.PROTECTED_VISIBILITY:
            value = "Protected";
            break;
          case ClassPane.DEFAULT_VISIBILITY:
            value = "Default";
            break;
          case ClassPane.PRIVATE_VISIBILITY:
            value = "Private";
            break;
          }
        }
        return super.getListCellRendererComponent(list, value, index, isSelected, cellHasFocus);
      }
    };
    JComboBox fieldVisibilityCombo = new JComboBox(new Integer[] {new Integer(ClassPane.PRIVATE_VISIBILITY), new Integer(ClassPane.DEFAULT_VISIBILITY), new Integer(ClassPane.PROTECTED_VISIBILITY), new Integer(ClassPane.PUBLIC_VISIBILITY)});
    fieldVisibilityCombo.setSelectedItem(new Integer(classPane.getFieldVisibility()));
    fieldVisibilityCombo.setRenderer(visibilityRenderer);
    gridBag.setConstraints(fieldVisibilityCombo, cons);
    visibilityPanel.add(fieldVisibilityCombo);
    fieldVisibilityCombo.addItemListener(new ItemListener() {
      public void itemStateChanged(ItemEvent e) {
        if(e.getStateChange() == ItemEvent.SELECTED) {
          classPane.setFieldVisibility(((Integer)e.getItem()).intValue());
        }
      }
    });
    cons.gridy++;
    JComboBox methodVisibilityCombo = new JComboBox(new Integer[] {new Integer(ClassPane.PRIVATE_VISIBILITY), new Integer(ClassPane.DEFAULT_VISIBILITY), new Integer(ClassPane.PROTECTED_VISIBILITY), new Integer(ClassPane.PUBLIC_VISIBILITY)});
    methodVisibilityCombo.setSelectedItem(new Integer(classPane.getMethodVisibility()));
    methodVisibilityCombo.setRenderer(visibilityRenderer);
    gridBag.setConstraints(methodVisibilityCombo, cons);
    visibilityPanel.add(methodVisibilityCombo);
    methodVisibilityCombo.addItemListener(new ItemListener() {
      public void itemStateChanged(ItemEvent e) {
        if(e.getStateChange() == ItemEvent.SELECTED) {
          classPane.setMethodVisibility(((Integer)e.getItem()).intValue());
        }
      }
    });
    filterPane.add(visibilityPanel, BorderLayout.SOUTH);
    updateListModel(list);
    return filterPane;
  }

  protected void updateListModel(JList list) {
    Filter[] filters = classPane.getFilters();
    Arrays.sort(filters, new Comparator() {
      public int compare(Object o1, Object o2) {
        return getFilterDisplay((Filter)o2).toLowerCase(Locale.ENGLISH).compareTo(getFilterDisplay((Filter)o1).toLowerCase(Locale.ENGLISH));
      }
    });
    Object[] values = list.getSelectedValues();
    DefaultListModel model = new DefaultListModel();
    list.setModel(model);
    for(int i=filters.length-1; i>=0; i--) {
      model.addElement(filters[i]);
    }
    for(int i=0; i<values.length; i++) {
      int index = model.indexOf(values[i]);
      if(index >= 0) {
        list.addSelectionInterval(index, index);
      }
    }
  }

  protected static String getFilterDisplay(Filter filter) {
    int modifiers = filter.getModifiers();
    StringBuffer sb = new StringBuffer();
    sb.append(filter.getNamePattern());
    boolean isClass = (modifiers & Modifiers.CLASS) != 0;
    boolean isInterface = (modifiers & Modifiers.INTERFACE) != 0;
    if(isInterface != isClass) {
      sb.append(" - ");
      if(isClass) {
        sb.append("classes");
      } else {
        sb.append("interfaces");
      }
    }
    return sb.toString();
  }

}
