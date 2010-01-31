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
import java.awt.Container;
import java.awt.Dialog;
import java.awt.Dimension;
import java.awt.FlowLayout;
import java.awt.Frame;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Insets;
import java.awt.Window;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;
import java.awt.event.ItemEvent;
import java.awt.event.ItemListener;
import java.awt.event.KeyAdapter;
import java.awt.event.KeyEvent;
import java.util.Arrays;
import java.util.Comparator;
import java.util.Locale;

import javax.swing.BorderFactory;
import javax.swing.DefaultListCellRenderer;
import javax.swing.DefaultListModel;
import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JComponent;
import javax.swing.JDialog;
import javax.swing.JLabel;
import javax.swing.JList;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JTextField;
import javax.swing.JTree;
import javax.swing.SwingUtilities;
import javax.swing.event.ListSelectionEvent;
import javax.swing.event.ListSelectionListener;
import javax.swing.event.TreeSelectionEvent;
import javax.swing.event.TreeSelectionListener;
import javax.swing.tree.DefaultMutableTreeNode;
import javax.swing.tree.DefaultTreeCellRenderer;
import javax.swing.tree.DefaultTreeModel;
import javax.swing.tree.TreePath;

import chrriis.udoc.model.ClassInfo;
import chrriis.udoc.model.Modifiers;
import chrriis.udoc.model.Util;
import chrriis.udoc.model.processor.ClassProcessor;
import chrriis.udoc.model.processor.ClassProcessorRegistry;
import chrriis.udoc.ui.ClassComponent;
import chrriis.udoc.ui.ClassPane;
import chrriis.udoc.ui.IconManager;
import chrriis.udoc.ui.JStateCheckBox;

public class ClassesToolBarMenu extends ToolBarMenu
{

	protected static final Color SELECTION_COLOR = new Color(218, 219, 255);

	protected ClassPane classPane;

	public ClassesToolBarMenu(ClassPane classPane)
	{
		super(IconManager.getIcon("tool_bar_on.gif"), IconManager.getIcon("tool_bar_off.gif"),
				classPane);
		this.classPane = classPane;
	}

	protected String getContentTitle()
	{
		return "Classes";
	}

	protected Color getSelectionColor()
	{
		return SELECTION_COLOR;
	}

	protected boolean isAdjustingCheckBox;

	protected Component createPopupMenuContent()
	{
		JPanel classVisibilityPane = new JPanel(new BorderLayout(2, 2));
		classVisibilityPane.setBorder(BorderFactory.createTitledBorder("Displayed Classes"));
		GridBagLayout gridBag = new GridBagLayout();
		JPanel classAdditionPane = new JPanel(gridBag);
		GridBagConstraints cons = new GridBagConstraints();
		cons.gridx = 0;
		cons.gridy = 0;
		JLabel classNameLabel = new JLabel("Classes: ");
		gridBag.setConstraints(classNameLabel, cons);
		classAdditionPane.add(classNameLabel);
		cons.gridx++;
		cons.weightx = 1.0;
		cons.fill = GridBagConstraints.HORIZONTAL;
		final JTextField classNameField = new JTextField(classPane.getLastClassNames(), 20);
		gridBag.setConstraints(classNameField, cons);
		classAdditionPane.add(classNameField);
		cons.gridx++;
		cons.weightx = 0.0;
		cons.fill = GridBagConstraints.NONE;
		final JButton browseClassesButton = new JButton("Browse...");
		gridBag.setConstraints(browseClassesButton, cons);
		classAdditionPane.add(browseClassesButton);
		cons.gridx++;
		JButton addClassesButton = new JButton("Add");
		gridBag.setConstraints(addClassesButton, cons);
		classAdditionPane.add(addClassesButton);
		classVisibilityPane.add(classAdditionPane, BorderLayout.NORTH);
		final JList list = new JList();
		list.setCellRenderer(new DefaultListCellRenderer()
		{
			public Component getListCellRendererComponent(JList list, Object value, int index,
					boolean isSelected, boolean cellHasFocus)
			{
				if (!(value instanceof ClassComponent))
				{
					return super.getListCellRendererComponent(list, value, index, isSelected,
							cellHasFocus);
				}
				ClassComponent classComponent = (ClassComponent) value;
				ClassInfo classInfo = classComponent.getClassInfo();
				String name = classInfo.getName();
				String packageName = classInfo.getPackage().getName();
				if (packageName.length() > 0)
				{
					name = name + " - " + packageName;
				}
				DefaultListCellRenderer renderer = (DefaultListCellRenderer) super
						.getListCellRendererComponent(list, name, index, isSelected, cellHasFocus);
				renderer.setIcon(IconManager.getIcon((classInfo.isLoaded() ? 0
						: Modifiers.NOT_LOADED)
						| classInfo.getModifiers()));
				renderer.setForeground(classComponent.isVisible() ? Color.BLACK : Color.GRAY);
				return renderer;
			}
		});
		JScrollPane scrollPane = new JScrollPane(list);
		scrollPane.setPreferredSize(new Dimension(200, 150));
		classVisibilityPane.add(scrollPane, BorderLayout.CENTER);
		JPanel southPanel = new JPanel(new FlowLayout(FlowLayout.CENTER, 0, 0));
		final JButton removeButton = new JButton("Remove");
		removeButton.setEnabled(false);
		removeButton.addActionListener(new ActionListener()
		{
			public void actionPerformed(ActionEvent e)
			{
				Object[] values = list.getSelectedValues();
				ClassComponent[] classComponents = new ClassComponent[values.length];
				System.arraycopy(values, 0, classComponents, 0, values.length);
				if (classPane.areClassComponentsDeletable(classComponents))
				{
					classPane.deleteClassComponents(classComponents);
					for (int i = 0; i < classComponents.length; i++)
					{
						((DefaultListModel) list.getModel()).removeElement(classComponents[i]);
					}
				}
			}
		});
		southPanel.add(removeButton);
		final JStateCheckBox checkBox = new JStateCheckBox("Filtered");
		southPanel.add(checkBox);
		classVisibilityPane.add(southPanel, BorderLayout.SOUTH);
		checkBox.addItemListener(new ItemListener()
		{
			public void itemStateChanged(ItemEvent e)
			{
				if (isAdjustingCheckBox)
				{
					return;
				}
				boolean isFiltered = e.getStateChange() == ItemEvent.SELECTED;
				checkBox.setGrayed(false);
				Object[] values = list.getSelectedValues();
				ClassComponent[] classComponents = new ClassComponent[values.length];
				System.arraycopy(values, 0, classComponents, 0, values.length);
				classPane.setClassComponentsFiltered(classComponents, isFiltered);
			}
		});
		list.addListSelectionListener(new ListSelectionListener()
		{
			public void valueChanged(ListSelectionEvent e)
			{
				boolean isVisible = false;
				boolean isInvisible = false;
				Object[] values = list.getSelectedValues();
				classPane.clearSelectedClassComponents();
				for (int i = 0; i < values.length; i++)
				{
					ClassComponent classComponent = (ClassComponent) values[i];
					if (classComponent.isVisible())
					{
						isVisible = true;
					}
					else
					{
						isInvisible = true;
					}
					if (isVisible && isInvisible)
					{
						break;
					}
					classPane.addSelectedClassComponent(classComponent);
				}
				isAdjustingCheckBox = true;
				checkBox.setGrayed(isVisible && isInvisible);
				checkBox.setSelected(!isVisible);
				checkBox.setEnabled(values.length > 0);
				isAdjustingCheckBox = false;
				ClassComponent[] classComponents = new ClassComponent[values.length];
				System.arraycopy(values, 0, classComponents, 0, values.length);
				removeButton.setEnabled(classPane.areClassComponentsDeletable(classComponents));
			}
		});
		checkBox.setEnabled(false);
		adjustList(list);
		GridBagLayout gridBag2 = new GridBagLayout();
		JPanel classProcessorPane = new JPanel(gridBag2);
		classProcessorPane.setBorder(BorderFactory.createTitledBorder("Processor"));
		GridBagConstraints cons2 = new GridBagConstraints();
		cons2.insets = new Insets(1, 0, 1, 0);
		cons2.gridx = 0;
		cons2.gridy = 0;
		cons2.anchor = GridBagConstraints.WEST;
		JLabel classProcessorLabel = new JLabel("Type: ");
		gridBag2.setConstraints(classProcessorLabel, cons2);
		classProcessorPane.add(classProcessorLabel);
		cons2.gridx++;
		cons2.weightx = 1.0;
		cons2.fill = GridBagConstraints.HORIZONTAL;
		ClassProcessor[] classProcessors = ClassProcessorRegistry.getClassProcessors();
		Arrays.sort(classProcessors, new Comparator()
		{
			public int compare(Object o1, Object o2)
			{
				return ((ClassProcessor) o1).getProcessorName().toLowerCase(Locale.ENGLISH)
						.compareTo(
								((ClassProcessor) o2).getProcessorName()
										.toLowerCase(Locale.ENGLISH));
			}
		});
		final JComboBox classProcessorComboBox = new JComboBox(classProcessors);
		classProcessorComboBox.setRenderer(new DefaultListCellRenderer()
		{
			public Component getListCellRendererComponent(JList list, Object value, int index,
					boolean isSelected, boolean cellHasFocus)
			{
				ClassProcessor classProcessor = (ClassProcessor) value;
				String displayedValue = classProcessor.getProcessorName();
				String description = classProcessor.getProcessorDescription();
				if (description != null)
				{
					displayedValue += " - " + description;
				}
				return super.getListCellRendererComponent(list, displayedValue, index, isSelected,
						cellHasFocus);
			}
		});
		gridBag2.setConstraints(classProcessorComboBox, cons2);
		classProcessorPane.add(classProcessorComboBox);
		cons2.gridx = 0;
		cons2.gridy++;
		cons2.gridwidth = 2;
		ActionListener actionListener = new ActionListener()
		{
			public void actionPerformed(ActionEvent e)
			{
				classPane.addClasses(classNameField.getText(),
						(ClassProcessor) classProcessorComboBox.getSelectedItem());
				adjustList(list);
			}
		};
		classNameField.addActionListener(actionListener);
		addClassesButton.addActionListener(actionListener);
		final JPanel processorParametersPanel = new JPanel(new BorderLayout(0, 0));
		processorParametersPanel.setVisible(false);
		gridBag2.setConstraints(processorParametersPanel, cons2);
		classProcessorPane.add(processorParametersPanel);
		classProcessorComboBox.addItemListener(new ItemListener()
		{
			public void itemStateChanged(ItemEvent e)
			{
				if (e.getStateChange() == ItemEvent.SELECTED)
				{
					ClassProcessor processor = (ClassProcessor) e.getItem();
					processorParametersPanel.removeAll();
					JComponent parametersComponent = processor.getParametersComponent();
					if (parametersComponent != null)
					{
						processorParametersPanel.add(parametersComponent, BorderLayout.CENTER);
					}
					processorParametersPanel.setVisible(parametersComponent != null);
					processorParametersPanel.revalidate();
					repaint();
				}
			}
		});
		classProcessorComboBox.setSelectedIndex(-1);
		classProcessorComboBox.setSelectedIndex(0);
		ClassProcessor lastClassProcessor = classPane.getLastClassProcessor();
		if (lastClassProcessor != null)
		{
			classProcessorComboBox.setSelectedItem(lastClassProcessor);
		}
		JPanel classPanel = new JPanel(new BorderLayout(0, 0))
		{
			public void requestFocus()
			{
				classNameField.requestFocus();
			}
		};
		classPanel.add(classVisibilityPane, BorderLayout.CENTER);
		classPanel.add(classProcessorPane, BorderLayout.NORTH);
		browseClassesButton.addActionListener(new ActionListener()
		{
			public void actionPerformed(ActionEvent e)
			{
				ClassProcessor classProcessor = (ClassProcessor) classProcessorComboBox
						.getSelectedItem();
				if (classProcessor == null)
				{
					return;
				}
				Window window = SwingUtilities.getWindowAncestor(ClassesToolBarMenu.this);
				final JDialog dialog;
				if (window instanceof Frame)
				{
					dialog = new JDialog((Frame) window);
				}
				else if (window instanceof Dialog)
				{
					dialog = new JDialog((Dialog) window);
				}
				else
				{
					return;
				}
				dialog.setTitle("Class Browser");
				Container contentPane = dialog.getContentPane();
				final JTree tree = new JTree(new DefaultTreeModel(new DefaultMutableTreeNode(
						"Classes")));
				tree.addKeyListener(new KeyAdapter()
				{
					public void keyPressed(KeyEvent e)
					{
						if (e.getKeyChar() != '*')
						{
							return;
						}
						TreePath[] selectionPaths = tree.getSelectionPaths();
						if (selectionPaths == null || selectionPaths.length == 0)
						{
							return;
						}
						for (int i = 0; i < tree.getRowCount(); i++)
						{
							TreePath path = tree.getPathForRow(i);
							for (int j = 0; j < selectionPaths.length; j++)
							{
								if (selectionPaths[j].isDescendant(path))
								{
									tree.expandRow(i);
									break;
								}
							}
						}
					}
				});
				tree.setRootVisible(false);
				tree.setShowsRootHandles(true);
				classProcessor.loadClassBrowser(tree);
				contentPane.add(new JScrollPane(tree), BorderLayout.CENTER);
				JPanel southPanel = new JPanel(new FlowLayout(FlowLayout.CENTER, 0, 2));
				final JButton addClassesButton = new JButton("Set");
				addClassesButton.setEnabled(false);
				tree.setCellRenderer(new DefaultTreeCellRenderer()
				{
					public Component getTreeCellRendererComponent(JTree tree, Object value,
							boolean sel, boolean expanded, boolean leaf, int row, boolean hasFocus)
					{
						DefaultTreeCellRenderer renderer = (DefaultTreeCellRenderer) super
								.getTreeCellRendererComponent(tree, value, sel, expanded, leaf,
										row, hasFocus);
						if (tree.getModel().getRoot() == value)
						{
							return renderer;
						}
						if (leaf)
						{
							renderer.setIcon(IconManager.getIcon("tree_type.gif"));
						}
						else
						{
							renderer.setIcon(IconManager.getIcon("tree_package.gif"));
						}
						return renderer;
					}
				});
				tree.addTreeSelectionListener(new TreeSelectionListener()
				{
					public void valueChanged(TreeSelectionEvent e)
					{
						addClassesButton.setEnabled(false);
						TreePath[] paths = tree.getSelectionPaths();
						if (paths == null)
						{
							return;
						}
						for (int i = 0; i < paths.length; i++)
						{
							if (((DefaultMutableTreeNode) paths[i].getLastPathComponent()).isLeaf())
							{
								addClassesButton.setEnabled(true);
								return;
							}
						}
					}
				});
				addClassesButton.addActionListener(new ActionListener()
				{
					public void actionPerformed(ActionEvent e)
					{
						TreePath[] paths = tree.getSelectionPaths();
						StringBuffer classesSB = new StringBuffer();
						boolean isAdded = false;
						for (int i = 0; i < paths.length; i++)
						{
							TreePath path = paths[i];
							if (((DefaultMutableTreeNode) path.getLastPathComponent()).isLeaf())
							{
								if (isAdded)
								{
									classesSB.append(' ');
								}
								else
								{
									isAdded = true;
								}
								Object[] objects = path.getPath();
								for (int j = 1; j < objects.length; j++)
								{
									if (j < objects.length - 1)
									{
										classesSB.append(
												((DefaultMutableTreeNode) objects[j])
														.getUserObject()).append('.');
									}
									else
									{
										classesSB
												.append(Util
														.escapeClassName((String) ((DefaultMutableTreeNode) objects[j])
																.getUserObject()));
									}
								}
							}
						}
						classNameField.setText(classesSB.toString());
						dialog.dispose();
					}
				});
				southPanel.add(addClassesButton);
				contentPane.add(southPanel, BorderLayout.SOUTH);
				dialog.setSize(new Dimension(300, 500));
				dialog.setLocationRelativeTo(browseClassesButton);
				dialog.setModal(true);
				dialog.setVisible(true);
			}
		});
		return classPanel;
	}

	protected void adjustList(JList list)
	{
		ClassComponent[] components = classPane.getClassComponents();
		Arrays.sort(components, new Comparator()
		{
			public int compare(Object o1, Object o2)
			{
				ClassInfo c1 = ((ClassComponent) o1).getClassInfo();
				ClassInfo c2 = ((ClassComponent) o2).getClassInfo();
				int m1 = c1.getModifiers();
				int m2 = c2.getModifiers();
				int result = 0;
				if ((m1 & Modifiers.INTERFACE) != 0)
				{
					if ((m2 & Modifiers.CLASS) != 0)
					{
						result = 1;
					}
					else
					{
						result = c1.getName().toLowerCase(Locale.ENGLISH).compareTo(
								c2.getName().toLowerCase(Locale.ENGLISH));
						if (result == 0)
						{
							result = c1.getPackage().getName().toLowerCase(Locale.ENGLISH)
									.compareTo(
											c2.getPackage().getName().toLowerCase(Locale.ENGLISH));
						}
					}
				}
				else if ((m2 & Modifiers.INTERFACE) != 0)
				{
					result = -1;
				}
				else
				{
					result = c1.getName().toLowerCase(Locale.ENGLISH).compareTo(
							c2.getName().toLowerCase(Locale.ENGLISH));
					if (result == 0)
					{
						result = c1.getPackage().getName().toLowerCase(Locale.ENGLISH).compareTo(
								c2.getPackage().getName().toLowerCase(Locale.ENGLISH));
					}
				}
				return -result;
			}
		});
		DefaultListModel model = new DefaultListModel();
		for (int i = components.length - 1; i >= 0; i--)
		{
			model.addElement(components[i]);
		}
		list.setModel(model);
		ClassComponent[] selectedClassComponents = classPane.getSelectedClassComponents();
		int[] selectedIndices = new int[selectedClassComponents.length];
		for (int i = 0; i < selectedClassComponents.length; i++)
		{
			selectedIndices[i] = model.indexOf(selectedClassComponents[i]);
		}
		list.setSelectedIndices(selectedIndices);
	}

}
