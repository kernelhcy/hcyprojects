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
import java.awt.Cursor;
import java.awt.Dimension;
import java.awt.FlowLayout;
import java.awt.Font;
import java.awt.Graphics;
import java.awt.GridBagConstraints;
import java.awt.GridBagLayout;
import java.awt.Point;
import java.awt.Rectangle;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import java.util.ArrayList;
import java.util.List;

import javax.swing.BorderFactory;
import javax.swing.Box;
import javax.swing.BoxLayout;
import javax.swing.JLabel;
import javax.swing.JPanel;
import javax.swing.JScrollPane;
import javax.swing.JSeparator;
import javax.swing.JViewport;
import javax.swing.Scrollable;
import javax.swing.SwingUtilities;
import javax.swing.event.MouseInputAdapter;

import chrriis.udoc.model.ClassInfo;
import chrriis.udoc.model.FieldInfo;
import chrriis.udoc.model.MethodInfo;
import chrriis.udoc.model.Modifiers;
import chrriis.udoc.model.PackageInfo;
import chrriis.udoc.ui.widgets.ClassLink;
import chrriis.udoc.ui.widgets.ClassTooltip;
import chrriis.udoc.ui.widgets.FieldDeclarationComponent;
import chrriis.udoc.ui.widgets.FieldLink;
import chrriis.udoc.ui.widgets.FieldTooltip;
import chrriis.udoc.ui.widgets.MethodDeclarationComponent;
import chrriis.udoc.ui.widgets.MethodLink;
import chrriis.udoc.ui.widgets.MethodTooltip;
import chrriis.udoc.ui.widgets.PackageLink;

public class ClassComponent extends JPanel
{

	protected static final Color OUTER_BORDER_COLOR = new Color(167, 166, 174);
	protected static final Color OUTER_BACKGROUND_COLOR = new Color(239, 238, 254);
	protected static final Color OUTER_BACKGROUND_SELECTION_COLOR = new Color(255, 230, 245);
	protected static final Color CLASS_BORDER_COLOR = OUTER_BORDER_COLOR;
	protected static final Color CLASS_BACKGROUND_COLOR = new Color(218, 219, 255);
	protected static final Font PACKAGE_FONT = new Font("sans-serif", Font.PLAIN, 10);
	protected static final Font TITLE_FONT = new Font("sans-serif", Font.BOLD, 12);

	protected ClassPane classPane;
	protected ClassInfo classInfo;
	protected JPanel packageComponent;
	protected JPanel centerPanel;
	protected MouseHandler moveMouseHandler = new MouseHandler();
	protected MouseHandler staticMouseHandler = new MouseHandler();

	protected InnerPanel innerPanel;
	protected JLabel showSuperTypesLabel;
	protected JLabel showSubTypesLabel;
	protected JLabel showCompositionsLabel;
	protected JLabel showAssociationsLabel;
	protected JPanel resizingPanel;

	protected Dimension expandedPreferredSize;
	protected Dimension collapsedPreferredSize;

	public Dimension getExpandedPreferredSize()
	{
		return expandedPreferredSize;
	}

	public void setExpandedPreferredSize(Dimension expandedPreferredSize)
	{
		this.expandedPreferredSize = expandedPreferredSize;
	}

	public ClassComponent(ClassPane classPane, ClassInfo classInfo)
	{
		super(new BorderLayout(0, 0));
		this.classPane = classPane;
		this.classInfo = classInfo;
		setOpaque(false);
		JPanel northPanel = new JPanel(new FlowLayout(FlowLayout.LEADING, 0, 0));
		northPanel.setOpaque(false);
		packageComponent = new JPanel(new FlowLayout(FlowLayout.LEADING, 0, 0));
		packageComponent.add(new JLabel(IconManager.getIcon("package.gif")));
		PackageInfo packageInfo = classInfo.getPackage();
		PackageLink packageNameLabel = new PackageLink(this, packageInfo.getName(), packageInfo);
		packageNameLabel.setFont(PACKAGE_FONT);
		packageComponent.setBackground(OUTER_BACKGROUND_COLOR);
		packageComponent.setBorder(BorderFactory.createMatteBorder(1, 1, 0, 1, OUTER_BORDER_COLOR));
		packageComponent.add(packageNameLabel);
		northPanel.add(packageComponent);
		add(northPanel, BorderLayout.NORTH);
		GridBagLayout gridBag = new GridBagLayout();
		centerPanel = new JPanel(gridBag);
		centerPanel.setBackground(OUTER_BACKGROUND_COLOR);
		GridBagConstraints cons = new GridBagConstraints();
		cons.gridx = 1;
		cons.gridy = 1;
		cons.weightx = 1.0;
		cons.weighty = 1.0;
		cons.fill = GridBagConstraints.BOTH;
		centerPanel.setBorder(BorderFactory.createLineBorder(OUTER_BORDER_COLOR));
		innerPanel = new InnerPanel(classInfo);
		gridBag.setConstraints(innerPanel, cons);
		centerPanel.add(innerPanel);
		showSuperTypesLabel = new JLabel()
		{
			public Cursor getCursor()
			{
				if (ClassComponent.this.classPane.isLocked()
						&& !ClassComponent.this.classInfo.isLoaded())
				{
					return Cursor.getDefaultCursor();
				}
				return super.getCursor();
			}
		};
		showSuperTypesLabel.addMouseListener(staticMouseHandler);
		showSuperTypesLabel.setCursor(Cursor.getPredefinedCursor(Cursor.HAND_CURSOR));
		showSuperTypesLabel.setToolTipText("Super Types");
		showSuperTypesLabel.addMouseListener(new MouseAdapter()
		{
			public void mousePressed(MouseEvent e)
			{
				if (e.getButton() != MouseEvent.BUTTON1 || ClassComponent.this.classPane.isLocked()
						&& !ClassComponent.this.classInfo.isLoaded())
				{
					return;
				}
				if (!isClassLoaded || ClassComponent.this.classInfo.getSuperTypes().length != 0)
				{
					setRelationsVisible(!areSuperTypesVisible, Relation.SUPER_TYPE);
				}
			}
		});
		setSuperTypesIndication(false);
		cons.gridy = 0;
		cons.weightx = 0.0;
		cons.weighty = 0.0;
		cons.fill = GridBagConstraints.NONE;
		gridBag.setConstraints(showSuperTypesLabel, cons);
		centerPanel.add(showSuperTypesLabel);
		showSubTypesLabel = new JLabel()
		{
			public Cursor getCursor()
			{
				if (ClassComponent.this.classPane.isLocked()
						&& !ClassComponent.this.classInfo.isLoaded())
				{
					return Cursor.getDefaultCursor();
				}
				return super.getCursor();
			}
		};
		showSubTypesLabel.addMouseListener(staticMouseHandler);
		showSubTypesLabel.setCursor(Cursor.getPredefinedCursor(Cursor.HAND_CURSOR));
		showSubTypesLabel.setToolTipText("Sub Types");
		showSubTypesLabel.addMouseListener(new MouseAdapter()
		{
			public void mousePressed(MouseEvent e)
			{
				if (e.getButton() != MouseEvent.BUTTON1 || ClassComponent.this.classPane.isLocked()
						&& !ClassComponent.this.classInfo.isLoaded())
				{
					return;
				}
				if (!isClassLoaded || ClassComponent.this.classInfo.getSubTypes().length != 0)
				{
					setRelationsVisible(!areSubTypesVisible, Relation.SUB_TYPE);
				}
			}
		});
		setSubTypesIndication(false);
		cons.gridy = 2;
		gridBag.setConstraints(showSubTypesLabel, cons);
		centerPanel.add(showSubTypesLabel);
		cons.gridy = 1;
		cons.gridx = 0;
		showCompositionsLabel = new JLabel()
		{
			public Cursor getCursor()
			{
				if (ClassComponent.this.classPane.isLocked()
						&& !ClassComponent.this.classInfo.isLoaded())
				{
					return Cursor.getDefaultCursor();
				}
				return super.getCursor();
			}
		};
		showCompositionsLabel.addMouseListener(staticMouseHandler);
		showCompositionsLabel.setCursor(Cursor.getPredefinedCursor(Cursor.HAND_CURSOR));
		showCompositionsLabel.setToolTipText("Compositions");
		showCompositionsLabel.addMouseListener(new MouseAdapter()
		{
			public void mousePressed(MouseEvent e)
			{
				if (e.getButton() != MouseEvent.BUTTON1 || ClassComponent.this.classPane.isLocked()
						&& !ClassComponent.this.classInfo.isLoaded())
				{
					return;
				}
				if (!isClassLoaded || ClassComponent.this.classInfo.getCompositions().length != 0)
				{
					setRelationsVisible(!areCompositionsVisible, Relation.COMPOSITION);
				}
			}
		});
		setCompositionsIndication(false);
		gridBag.setConstraints(showCompositionsLabel, cons);
		centerPanel.add(showCompositionsLabel);
		cons.gridx = 2;
		showAssociationsLabel = new JLabel()
		{
			public Cursor getCursor()
			{
				if (ClassComponent.this.classPane.isLocked()
						&& !ClassComponent.this.classInfo.isLoaded())
				{
					return Cursor.getDefaultCursor();
				}
				return super.getCursor();
			}
		};
		showAssociationsLabel.addMouseListener(staticMouseHandler);
		showAssociationsLabel.setCursor(Cursor.getPredefinedCursor(Cursor.HAND_CURSOR));
		showAssociationsLabel.setToolTipText("Associations");
		showAssociationsLabel.addMouseListener(new MouseAdapter()
		{
			public void mousePressed(MouseEvent e)
			{
				if (e.getButton() != MouseEvent.BUTTON1 || ClassComponent.this.classPane.isLocked()
						&& !ClassComponent.this.classInfo.isLoaded())
				{
					return;
				}
				if (!isClassLoaded || ClassComponent.this.classInfo.getAssociations().length != 0)
				{
					setRelationsVisible(!areAssociationsVisible, Relation.ASSOCIATION);
				}
			}
		});
		setAssociationsIndication(false);
		gridBag.setConstraints(showAssociationsLabel, cons);
		centerPanel.add(showAssociationsLabel);
		cons.gridy++;
		cons.fill = GridBagConstraints.BOTH;
		resizingPanel = new JPanel(null)
		{
			protected void paintComponent(Graphics g)
			{
				super.paintComponent(g);
				Dimension size = getSize();
				int max = Math.max(size.width, size.height);
				g.setColor(OUTER_BORDER_COLOR);
				for (int i = 0; i < max; i += 2)
				{
					g.drawLine(max, i, i, max);
				}
			}
		};
		resizingPanel.setOpaque(false);
		resizingPanel.setVisible(false);
		resizingPanel.setCursor(Cursor.getPredefinedCursor(Cursor.SE_RESIZE_CURSOR));
		MouseInputAdapter resizingMouseEvent = new MouseInputAdapter()
		{
			protected Point location;

			public void mouseClicked(MouseEvent e)
			{
				if (e.getClickCount() == 2)
				{
					Dimension size = getPreferredSize();
					expandedPreferredSize = size;
					setSize(size);
					revalidate();
					ClassComponent.this.classPane.adjustBounds();
					ClassComponent.this.classPane.getClassComponentPane().repaint();
				}
			}

			public void mousePressed(MouseEvent e)
			{
				if (e.getButton() != MouseEvent.BUTTON1)
				{
					return;
				}
				processMousePressed(e);
				Point p = new Point(e.getPoint());
				location = SwingUtilities.convertPoint(resizingPanel, p,
						ClassComponent.this.classPane);
			}

			public void mouseDragged(MouseEvent e)
			{
				if (location == null)
				{
					return;
				}
				Dimension size = getSize();
				Point p = new Point(e.getPoint());
				p = SwingUtilities.convertPoint(resizingPanel, p, ClassComponent.this.classPane);
				size.width += p.x - location.x;
				size.height += p.y - location.y;
				if (size.width < collapsedPreferredSize.width)
				{
					p.x -= size.width - collapsedPreferredSize.width;
					size.width = collapsedPreferredSize.width;
				}
				if (size.height < collapsedPreferredSize.height)
				{
					p.y -= size.height - collapsedPreferredSize.height;
					size.height = collapsedPreferredSize.height;
				}
				location = p;
				expandedPreferredSize = size;
				setSize(size);
				revalidate();
				ClassComponent.this.classPane.adjustBounds();
				ClassComponent.this.classPane.getClassComponentPane().repaint();
			}

			public void mouseReleased(MouseEvent e)
			{
				if (e.getButton() != MouseEvent.BUTTON1)
				{
					return;
				}
				location = null;
			}
		};
		resizingPanel.addMouseMotionListener(resizingMouseEvent);
		resizingPanel.addMouseListener(resizingMouseEvent);
		gridBag.setConstraints(resizingPanel, cons);
		centerPanel.add(resizingPanel);
		add(centerPanel, BorderLayout.CENTER);
		addMouseListener(moveMouseHandler);
		addMouseMotionListener(moveMouseHandler);
		collapsedPreferredSize = getPreferredSize();
		setSize(collapsedPreferredSize);
		if (classInfo.isLoaded())
		{
			setClassLoaded();
		}
		else
		{
			autoLoad();
		}
	}

	protected void autoLoad()
	{
		if (classPane.isAutoLoading())
		{
			new Thread()
			{
				public void run()
				{
					if (!classPane.isAutoLoading())
					{
						return;
					}
					load();
				}
			}.start();
		}
	}

	protected void reload()
	{
		if (isReloadable())
		{
			ClassInfo classInfo = getClassInfo();
			innerPanel.setClassLoading();
			classInfo.load(true);
			SwingUtilities.invokeLater(new Runnable()
			{
				public void run()
				{
					setClassLoaded();
				}
			});
		}
	}

	protected boolean isReloadable()
	{
		ClassInfo classInfo = getClassInfo();
		return classInfo.getLoadingState() != ClassInfo.LOADED_STATE;
	}

	protected void load()
	{
		ClassInfo classInfo = getClassInfo();
		if (!classInfo.isLoaded())
		{
			innerPanel.setClassLoading();
			classInfo.load(false);
			SwingUtilities.invokeLater(new Runnable()
			{
				public void run()
				{
					setClassLoaded();
				}
			});
		}
	}

	protected boolean isSelected;

	public boolean isSelected()
	{
		return isSelected;
	}

	protected void setSelected(boolean isSelected)
	{
		this.isSelected = isSelected;
		if (isSelected)
		{
			packageComponent.setBackground(OUTER_BACKGROUND_SELECTION_COLOR);
			centerPanel.setBackground(OUTER_BACKGROUND_SELECTION_COLOR);
		}
		else
		{
			packageComponent.setBackground(OUTER_BACKGROUND_COLOR);
			centerPanel.setBackground(OUTER_BACKGROUND_COLOR);
		}
	}

	public void print(Graphics g)
	{
		boolean isSelected = isSelected();
		setSelected(false);
		boolean isResizingPanelVisible = resizingPanel.isVisible();
		resizingPanel.setVisible(false);
		super.print(g);
		setSelected(isSelected);
		resizingPanel.setVisible(isResizingPanelVisible);
	}

	protected boolean areSuperTypesVisible;
	protected boolean areSubTypesVisible;
	protected boolean areCompositionsVisible;
	protected boolean areAssociationsVisible;

	protected boolean areSuperTypesVisible()
	{
		return areSuperTypesVisible;
	}

	protected void setSuperTypesIndication(boolean areSuperTypesVisible)
	{
		this.areSuperTypesVisible = areSuperTypesVisible;
		if (!isClassLoaded || classInfo.getSuperTypes().length != 0)
		{
			showSuperTypesLabel.setIcon(IconManager
					.getIcon(areSuperTypesVisible ? "super_types_on.gif" : "super_types_off.gif"));
		}
	}

	protected boolean areSubTypesVisible()
	{
		return areSubTypesVisible;
	}

	protected void setSubTypesIndication(boolean areSubTypesVisible)
	{
		this.areSubTypesVisible = areSubTypesVisible;
		showSubTypesLabel.setIcon(IconManager.getIcon(areSubTypesVisible ? "sub_types_on.gif"
				: "sub_types_off.gif"));
	}

	protected boolean areCompositionsVisible()
	{
		return areCompositionsVisible;
	}

	protected void setCompositionsIndication(boolean areCompositionsVisible)
	{
		this.areCompositionsVisible = areCompositionsVisible;
		showCompositionsLabel.setIcon(IconManager
				.getIcon(areCompositionsVisible ? "compositions_on.gif" : "compositions_off.gif"));
	}

	protected boolean areAssociationsVisible()
	{
		return areAssociationsVisible;
	}

	protected void setAssociationsIndication(boolean areAssociationsVisible)
	{
		this.areAssociationsVisible = areAssociationsVisible;
		showAssociationsLabel.setIcon(IconManager
				.getIcon(areAssociationsVisible ? "associations_on.gif" : "associations_off.gif"));
	}

	protected volatile boolean isClassLoaded;

	protected void setClassLoaded()
	{
		if (isClassLoaded)
		{
			return;
		}
		innerPanel.setClassLoaded();
		if (classInfo.getLoadingState() == ClassInfo.LOADING_FAILED_STATE)
		{
			return;
		}
		isClassLoaded = true;
		if (classInfo.getSuperTypes().length == 0)
		{
			showSuperTypesLabel.setIcon(IconManager.getIcon("super_types_none.gif"));
			showSuperTypesLabel.removeMouseListener(staticMouseHandler);
			showSuperTypesLabel.addMouseListener(moveMouseHandler);
			showSuperTypesLabel.addMouseMotionListener(moveMouseHandler);
			showSuperTypesLabel.setCursor(null);
			showSuperTypesLabel.setToolTipText(null);
		}
		if (classInfo.getSubTypes().length == 0)
		{
			showSubTypesLabel.setIcon(IconManager.getIcon("sub_types_none.gif"));
			showSubTypesLabel.removeMouseListener(staticMouseHandler);
			showSubTypesLabel.addMouseListener(moveMouseHandler);
			showSubTypesLabel.addMouseMotionListener(moveMouseHandler);
			showSubTypesLabel.setCursor(null);
			showSubTypesLabel.setToolTipText(null);
		}
		if (classInfo.getCompositions().length == 0)
		{
			showCompositionsLabel.setIcon(IconManager.getIcon("compositions_none.gif"));
			showCompositionsLabel.removeMouseListener(staticMouseHandler);
			showCompositionsLabel.addMouseListener(moveMouseHandler);
			showCompositionsLabel.addMouseMotionListener(moveMouseHandler);
			showCompositionsLabel.setCursor(null);
			showCompositionsLabel.setToolTipText(null);
		}
		if (classInfo.getAssociations().length == 0)
		{
			showAssociationsLabel.setIcon(IconManager.getIcon("associations_none.gif"));
			showAssociationsLabel.removeMouseListener(staticMouseHandler);
			showAssociationsLabel.addMouseListener(moveMouseHandler);
			showAssociationsLabel.addMouseMotionListener(moveMouseHandler);
			showAssociationsLabel.setCursor(null);
			showAssociationsLabel.setToolTipText(null);
		}
		resizingPanel.setVisible(innerPanel.isExpanded);
		collapsedPreferredSize = getPreferredSize();
		classPane.repaint();
	}

	public ClassInfo getClassInfo()
	{
		return classInfo;
	}

	public void setExpanded(boolean isExpanded)
	{
		resizingPanel.setVisible(isExpanded && isClassLoaded && hasClassContent());
		innerPanel.setExpanded(isExpanded);
		innerPanel.setSize(innerPanel.getPreferredSize());
		setSize(expandedPreferredSize != null && isExpanded ? expandedPreferredSize
				: getPreferredSize());
		revalidate();
		repaint();
	}

	public boolean isExpanded()
	{
		return innerPanel.isExpanded();
	}

	public void updateFieldVisibility()
	{
		innerPanel.updateMembersPanel();
	}

	public void updateMethodVisibility()
	{
		innerPanel.updateMembersPanel();
	}

	public boolean contains(int x, int y)
	{
		if (packageComponent.contains(x, y))
		{
			return true;
		}
		Point location = centerPanel.getLocation();
		return centerPanel.contains(x - location.x, y - location.y);
	}

	class MouseHandler extends MouseInputAdapter
	{
		protected Point referenceLocation;

		public void mouseDragged(MouseEvent e)
		{
			if (referenceLocation == null)
			{
				return;
			}
			classPane.moveSelectedClassComponent(ClassComponent.this, e.getX()
					- referenceLocation.x, e.getY() - referenceLocation.y);
		}

		public void mousePressed(MouseEvent e)
		{
			if (e.getButton() != MouseEvent.BUTTON1)
			{
				return;
			}
			referenceLocation = e.getPoint();
			processMousePressed(e);
		}

		public void mouseReleased(MouseEvent e)
		{
			if (e.getButton() != MouseEvent.BUTTON1)
			{
				return;
			}
			referenceLocation = null;
			if ((e.getModifiers() & (MouseEvent.BUTTON2_MASK | MouseEvent.BUTTON3_MASK)) != 0)
			{
				return;
			}
			showTooltip(e, true);
		}

		public void mouseClicked(MouseEvent e)
		{
			if (e.getButton() != MouseEvent.BUTTON1)
			{
				return;
			}
			if (isSelected && (e.getModifiers() & MouseEvent.CTRL_MASK) == 0)
			{
				classPane.setSelectedClassComponent(ClassComponent.this);
			}
			showTooltip(e, false);
		}

		public void mouseExited(MouseEvent e)
		{
			classPane.hideTooltip();
			// System.err.println("1");
		}

		// public void mouseClicked(MouseEvent e) {
		// showTooltip(e, false);
		// }
		protected void showTooltip(MouseEvent e, boolean isForced)
		{
			// System.err.println("2");
			if (!isForced
					&& (e.getModifiers() & (MouseEvent.BUTTON1_MASK | MouseEvent.BUTTON2_MASK | MouseEvent.BUTTON3_MASK)) != 0)
			{
				return;
			}
			Point p = SwingUtilities.convertPoint(e.getComponent(), e.getPoint(),
					ClassComponent.this);
			Component component = ClassComponent.this.findComponentAt(p);
			if (component != null)
			{
				if (component != ClassComponent.this)
				{
					if (component instanceof ClassLink)
					{
						ClassLink classLink = (ClassLink) component;
						ClassInfo classInfo = classLink.getClassInfo();
						if (classInfo.isLoaded())
						{
							String prototype = classInfo.getPrototype();
							if (prototype != null)
							{
								ClassTooltip tooltip = new ClassTooltip(classInfo);
								classPane
										.showTooltip(
												tooltip,
												classLink,
												(classLink.getWidth() - tooltip.getPreferredSize().width) / 2,
												classLink.getHeight() + 20);
							}
						}
					}
					else if (component instanceof MethodLink)
					{
						MethodLink methodLink = (MethodLink) component;
						MethodInfo methodInfo = methodLink.getMethodInfo();
						String prototype = methodInfo.getPrototype();
						if (prototype != null)
						{
							MethodTooltip tooltip = new MethodTooltip(methodInfo);
							classPane.showTooltip(tooltip, methodLink,
									(methodLink.getWidth() - tooltip.getPreferredSize().width) / 2,
									methodLink.getHeight() + 20);
						}
					}
					else if (component instanceof FieldLink)
					{
						FieldLink fieldLink = (FieldLink) component;
						FieldInfo fieldInfo = fieldLink.getFieldInfo();
						String prototype = fieldInfo.getPrototype();
						if (prototype != null)
						{
							FieldTooltip tooltip = new FieldTooltip(fieldInfo);
							classPane.showTooltip(tooltip, fieldLink,
									(fieldLink.getWidth() - tooltip.getPreferredSize().width) / 2,
									fieldLink.getHeight() + 20);
						}
					}
				}
			}
		}
	}

	protected void processMousePressed(MouseEvent e)
	{
		classPane.closeMenus();
		ClassComponent classComponent = ClassComponent.this;
		Container parent = classComponent.getParent();
		parent.setComponentZOrder(classComponent, 0);
		if ((e.getModifiers() & MouseEvent.CTRL_MASK) != 0)
		{
			if (classPane.isClassComponentSelected(classComponent))
			{
				classPane.removeSelectedClassComponent(classComponent);
			}
			else
			{
				classPane.addSelectedClassComponent(classComponent);
			}
		}
		else if (!classPane.isClassComponentSelected(classComponent))
		{
			classPane.setSelectedClassComponent(classComponent);
		}
		classPane.repaint();
	}

	class InnerPanel extends JPanel
	{
		protected ClassInfo classInfo;
		protected JLabel titleLabel;
		protected JLabel expandLabel;

		public InnerPanel(ClassInfo classInfo)
		{
			super(new BorderLayout(0, 0));
			this.classInfo = classInfo;
			setOpaque(true);
			setBackground(CLASS_BACKGROUND_COLOR);
			setBorder(BorderFactory.createLineBorder(CLASS_BORDER_COLOR));
			JPanel northPanel = new JPanel(new BorderLayout(0, 0));
			northPanel.setBackground(CLASS_BACKGROUND_COLOR);
			JPanel titlePanel = new JPanel(new FlowLayout(FlowLayout.CENTER, 0, 0));
			titlePanel.setBackground(CLASS_BACKGROUND_COLOR);
			expandLabel = new JLabel(IconManager.getIcon("expand.gif"))
			{
				public boolean isVisible()
				{
					if (ClassComponent.this.classPane.isLocked()
							&& !InnerPanel.this.classInfo.isLoaded())
					{
						return false;
					}
					return super.isVisible();
				}
			};
			expandLabel.setCursor(Cursor.getPredefinedCursor(Cursor.HAND_CURSOR));
			expandLabel.addMouseListener(staticMouseHandler);
			expandLabel.addMouseListener(new MouseAdapter()
			{
				public void mousePressed(MouseEvent e)
				{
					if (e.getButton() != MouseEvent.BUTTON1
							|| ClassComponent.this.classPane.isLocked()
							&& !InnerPanel.this.classInfo.isLoaded())
					{
						return;
					}
					ClassComponent.this.setExpanded(!isExpanded);
				}
			});
			northPanel.add(expandLabel, BorderLayout.WEST);
			titleLabel = new JLabel(IconManager.getIcon(Modifiers.NOT_LOADED
					| classInfo.getModifiers()));
			titleLabel.setFont(TITLE_FONT);
			titlePanel.add(titleLabel);
			titlePanel.add(new ClassLink(ClassComponent.this, classInfo.getName(), classInfo));
			northPanel.add(titlePanel, BorderLayout.CENTER);
			add(northPanel, BorderLayout.NORTH);
		}

		protected JPanel centerPanel;
		protected JPanel membersPanel;
		protected boolean isExpanded;

		public boolean isExpanded()
		{
			return isExpanded;
		}

		public void setExpanded(boolean isExpanded)
		{
			if (!hasClassContent()
					&& (isClassLoaded || (classInfo.getModifiers() & Modifiers.LOADING_FAILED) != 0))
			{
				isExpanded = false;
			}
			expandLabel.setIcon(isExpanded ? IconManager.getIcon("collapse.gif") : IconManager
					.getIcon("expand.gif"));
			if (this.isExpanded == isExpanded)
			{
				return;
			}
			if (centerPanel != null)
			{
				this.isExpanded = isExpanded;
				centerPanel.setVisible(isExpanded);
				classPane.setDevelopped(ClassComponent.this, isExpanded);
				return;
			}
			if (!isExpanded)
			{
				this.isExpanded = isExpanded;
				classPane.setDevelopped(ClassComponent.this, isExpanded);
				return;
			}
			this.isExpanded = isExpanded;
			if (!classInfo.isLoaded())
			{
				new Thread()
				{
					public void run()
					{
						load();
						SwingUtilities.invokeLater(new Runnable()
						{
							public void run()
							{
								if (InnerPanel.this.isExpanded)
								{
									InnerPanel.this.isExpanded = false;
									ClassComponent.this.setExpanded(true);
								}
							}
						});
					}
				}.start();
				return;
			}
			centerPanel = new JPanel(new BorderLayout(0, 0));
			centerPanel.setBackground(CLASS_BACKGROUND_COLOR);
			updateMembersPanel();
			JScrollPane scrollPane = new JScrollPane(membersPanel)
			{
				public Dimension getPreferredSize()
				{
					Dimension size = getViewport().getView().getPreferredSize();
					size.height++;
					Dimension preferredSize = new Dimension(Math.min(200, size.width), Math.min(
							200, size.height));
					if (size.height < 200 && size.width > 200)
					{
						preferredSize.height += getHorizontalScrollBar().getPreferredSize().height;
					}
					if (size.width < 200 && size.height > 200)
					{
						preferredSize.width += getVerticalScrollBar().getPreferredSize().width;
					}
					return preferredSize;
				}
			};
			JViewport viewport = scrollPane.getViewport();
			viewport.addMouseListener(moveMouseHandler);
			viewport.addMouseMotionListener(moveMouseHandler);
			scrollPane.setBorder(BorderFactory.createMatteBorder(1, 0, 0, 0, CLASS_BORDER_COLOR));
			centerPanel.add(scrollPane, BorderLayout.CENTER);
			add(centerPanel, BorderLayout.CENTER);
			classPane.setDevelopped(ClassComponent.this, isExpanded);
		}

		protected FieldInfo[] filterFieldInfos(FieldInfo[] fieldInfos, int visibility)
		{
			if (visibility == ClassPane.PRIVATE_VISIBILITY)
			{
				return fieldInfos;
			}
			List fieldInfoList = new ArrayList(fieldInfos.length);
			for (int i = 0; i < fieldInfos.length; i++)
			{
				FieldInfo fieldInfo = fieldInfos[i];
				int modifiers = fieldInfo.getModifiers();
				switch (visibility)
				{
				case ClassPane.PUBLIC_VISIBILITY:
					if ((modifiers & Modifiers.PUBLIC) != 0)
					{
						fieldInfoList.add(fieldInfo);
					}
					break;
				case ClassPane.PROTECTED_VISIBILITY:
					if ((modifiers & (Modifiers.PUBLIC | Modifiers.PROTECTED)) != 0)
					{
						fieldInfoList.add(fieldInfo);
					}
					break;
				case ClassPane.DEFAULT_VISIBILITY:
					if ((modifiers & (Modifiers.PUBLIC | Modifiers.PROTECTED | Modifiers.DEFAULT)) != 0)
					{
						fieldInfoList.add(fieldInfo);
					}
					break;
				}
			}
			return (FieldInfo[]) fieldInfoList.toArray(new FieldInfo[0]);
		}

		protected MethodInfo[] filterMethodInfos(MethodInfo[] methodInfos, int visibility)
		{
			if (visibility == ClassPane.PRIVATE_VISIBILITY)
			{
				return methodInfos;
			}
			List methodInfoList = new ArrayList(methodInfos.length);
			for (int i = 0; i < methodInfos.length; i++)
			{
				MethodInfo methodInfo = methodInfos[i];
				int modifiers = methodInfo.getModifiers();
				switch (visibility)
				{
				case ClassPane.PUBLIC_VISIBILITY:
					if ((modifiers & Modifiers.PUBLIC) != 0)
					{
						methodInfoList.add(methodInfo);
					}
					break;
				case ClassPane.PROTECTED_VISIBILITY:
					if ((modifiers & (Modifiers.PUBLIC | Modifiers.PROTECTED)) != 0)
					{
						methodInfoList.add(methodInfo);
					}
					break;
				case ClassPane.DEFAULT_VISIBILITY:
					if ((modifiers & (Modifiers.PUBLIC | Modifiers.PROTECTED | Modifiers.DEFAULT)) != 0)
					{
						methodInfoList.add(methodInfo);
					}
					break;
				}
			}
			return (MethodInfo[]) methodInfoList.toArray(new MethodInfo[0]);
		}

		public void updateMembersPanel()
		{
			int fieldVisibility = classPane.getFieldVisibility();
			FieldInfo[] enums = filterFieldInfos(classInfo.getEnums(), fieldVisibility);
			FieldInfo[] fields = filterFieldInfos(classInfo.getFields(), fieldVisibility);
			int methodVisibility = classPane.getMethodVisibility();
			MethodInfo[] constructors = filterMethodInfos(classInfo.getConstructors(),
					methodVisibility);
			MethodInfo[] methods = filterMethodInfos(classInfo.getMethods(), methodVisibility);
			MethodInfo[] annotationMembers = filterMethodInfos(classInfo.getAnnotationMembers(),
					methodVisibility);
			class MembersPanel extends JPanel implements Scrollable
			{
				public Dimension getPreferredScrollableViewportSize()
				{
					Dimension preferredSize = getPreferredSize();
					if (getComponentCount() > 0)
					{
						preferredSize.height = getComponent(0).getPreferredSize().height * 3;
					}
					return preferredSize;
				}

				public int getScrollableUnitIncrement(Rectangle visibleRect, int orientation,
						int direction)
				{
					if (getComponentCount() > 0)
					{
						return getComponent(0).getSize().height;
					}
					return 0;
				}

				public int getScrollableBlockIncrement(Rectangle visibleRect, int orientation,
						int direction)
				{
					if (getComponentCount() > 0)
					{
						return getComponent(0).getSize().height * 3;
					}
					return 0;
				}

				public boolean getScrollableTracksViewportWidth()
				{
					return getPreferredSize().width < getParent().getWidth();
				}

				public boolean getScrollableTracksViewportHeight()
				{
					return getPreferredSize().height < getParent().getHeight();
				}
			}
			if (membersPanel == null)
			{
				membersPanel = new MembersPanel();
				BoxLayout boxLayout = new BoxLayout(membersPanel, BoxLayout.Y_AXIS);
				membersPanel.setLayout(boxLayout);
			}
			else
			{
				membersPanel.removeAll();
			}
			membersPanel.setBackground(CLASS_BACKGROUND_COLOR);
			boolean hasAdded = false;
			for (int i = 0; i < enums.length; i++)
			{
				membersPanel.add(new FieldDeclarationComponent(ClassComponent.this, classInfo,
						enums[i]));
				hasAdded = true;
			}
			if (fields.length > 0)
			{
				if (hasAdded)
				{
					membersPanel.add(new JSeparator(JSeparator.HORIZONTAL)
					{
						public Dimension getMaximumSize()
						{
							return new Dimension(super.getMaximumSize().width,
									getPreferredSize().height);
						}
					});
					hasAdded = false;
				}
				for (int i = 0; i < fields.length; i++)
				{
					membersPanel.add(new FieldDeclarationComponent(ClassComponent.this, classInfo,
							fields[i]));
					hasAdded = true;
				}
			}
			if (constructors.length > 0)
			{
				if (hasAdded)
				{
					membersPanel.add(new JSeparator(JSeparator.HORIZONTAL)
					{
						public Dimension getMaximumSize()
						{
							return new Dimension(super.getMaximumSize().width,
									getPreferredSize().height);
						}
					});
					hasAdded = false;
				}
				for (int i = 0; i < constructors.length; i++)
				{
					membersPanel.add(new MethodDeclarationComponent(ClassComponent.this, classInfo,
							constructors[i]));
					hasAdded = true;
				}
			}
			if (methods.length > 0)
			{
				if (hasAdded)
				{
					membersPanel.add(new JSeparator(JSeparator.HORIZONTAL)
					{
						public Dimension getMaximumSize()
						{
							return new Dimension(super.getMaximumSize().width,
									getPreferredSize().height);
						}
					});
					hasAdded = false;
				}
				for (int i = 0; i < methods.length; i++)
				{
					membersPanel.add(new MethodDeclarationComponent(ClassComponent.this, classInfo,
							methods[i]));
					hasAdded = true;
				}
			}
			if (annotationMembers.length > 0)
			{
				if (hasAdded)
				{
					membersPanel.add(new JSeparator(JSeparator.HORIZONTAL)
					{
						public Dimension getMaximumSize()
						{
							return new Dimension(super.getMaximumSize().width,
									getPreferredSize().height);
						}
					});
					hasAdded = false;
				}
				for (int i = 0; i < annotationMembers.length; i++)
				{
					membersPanel.add(new MethodDeclarationComponent(ClassComponent.this, classInfo,
							annotationMembers[i]));
					hasAdded = true;
				}
			}
			membersPanel.add(Box.createVerticalGlue());
			membersPanel.revalidate();
			membersPanel.repaint();
		}

		public void setClassLoaded()
		{
			int modifiers = 0;
			boolean hasFailed = classInfo.getLoadingState() == ClassInfo.LOADING_FAILED_STATE;
			if (hasFailed)
			{
				modifiers |= Modifiers.LOADING_FAILED;
			}
			titleLabel.setIcon(IconManager.getIcon(modifiers | classInfo.getModifiers()));
			if (!hasFailed && !hasClassContent())
			{
				ClassComponent.this.setExpanded(false);
				expandLabel.setVisible(false);
			}
		}

		public void setClassLoading()
		{
			titleLabel.setIcon(IconManager.getIcon(Modifiers.LOADING | classInfo.getModifiers()
					& ~Modifiers.LOADING_FAILED));
		}

	}

	protected boolean hasClassContent()
	{
		return classInfo.getEnums().length != 0 || classInfo.getAnnotationMembers().length != 0
				|| classInfo.getFields().length != 0 || classInfo.getConstructors().length != 0
				|| classInfo.getMethods().length != 0;
	}

	public Point getCenter()
	{
		Rectangle bounds = innerPanel.getBounds();
		bounds = SwingUtilities.convertRectangle(innerPanel.getParent(), bounds, getParent());
		return new Point(bounds.x + bounds.width / 2, bounds.y + bounds.height / 2);
	}

	public Point getRelationPoint(Point p2)
	{
		Rectangle cBounds = centerPanel.getBounds();
		cBounds = SwingUtilities.convertRectangle(centerPanel.getParent(), cBounds, getParent());
		cBounds.x--;
		cBounds.y--;
		cBounds.width++;
		cBounds.height++;
		Point p1 = new Point(cBounds.x + cBounds.width / 2, cBounds.y + cBounds.height / 2);
		Rectangle pBounds = packageComponent.getBounds();
		pBounds = SwingUtilities.convertRectangle(packageComponent.getParent(), pBounds,
				getParent());
		pBounds.x--;
		pBounds.y--;
		pBounds.width++;
		Point q1 = new Point(pBounds.x, pBounds.y);
		Point q2 = new Point(pBounds.x + pBounds.width, pBounds.y);
		Point q3 = new Point(pBounds.x + pBounds.width, pBounds.y + pBounds.height);
		Point q4 = new Point(pBounds.x + cBounds.width, pBounds.y + pBounds.height);
		Point q5 = new Point(pBounds.x + cBounds.width, cBounds.y + cBounds.height);
		Point q6 = new Point(pBounds.x, cBounds.y + cBounds.height);
		Point intersection = null;
		if (intersection == null)
		{
			intersection = getIntersection(p1, p2, q1, q2);
		}
		if (intersection == null)
		{
			intersection = getIntersection(p1, p2, q2, q3);
		}
		if (intersection == null)
		{
			intersection = getIntersection(p1, p2, q3, q4);
		}
		if (intersection == null)
		{
			intersection = getIntersection(p1, p2, q4, q5);
		}
		if (intersection == null)
		{
			intersection = getIntersection(p1, p2, q5, q6);
		}
		if (intersection == null)
		{
			intersection = getIntersection(p1, p2, q6, q1);
		}
		return intersection;
	}

	protected Point getIntersection(Point p1, Point p2, Point p3, Point p4)
	{
		float uan = (p4.x - p3.x) * (p1.y - p3.y) - (p4.y - p3.y) * (p1.x - p3.x);
		if (uan == 0)
		{
			return null;
		}
		float uad = (p4.y - p3.y) * (p2.x - p1.x) - (p4.x - p3.x) * (p2.y - p1.y);
		if (uad == 0)
		{
			return null;
		}
		float ua = uan / uad;
		Point p = new Point(Math.round(p1.x + ua * (p2.x - p1.x)), Math.round(p1.y + ua
				* (p2.y - p1.y)));
		if (p.x < p1.x && p.x < p2.x || p.x < p3.x && p.x < p4.x || p.x > p1.x && p.x > p2.x
				|| p.x > p3.x && p.x > p4.x || p.y < p1.y && p.y < p2.y || p.y < p3.y && p.y < p4.y
				|| p.y > p1.y && p.y > p2.y || p.y > p3.y && p.y > p4.y)
		{
			return null;
		}
		return p;
	}

	protected void setRelationsVisible(boolean areRelationsVisible, final int relationType)
	{
		final ClassInfo classInfo = getClassInfo();
		ClassInfo[] relatedClassInfos = null;
		switch (relationType)
		{
		case Relation.SUPER_TYPE:
			relatedClassInfos = classInfo.getSuperTypes();
			break;
		case Relation.SUB_TYPE:
			relatedClassInfos = classInfo.getSubTypes();
			break;
		case Relation.COMPOSITION:
			relatedClassInfos = classInfo.getCompositions();
			break;
		case Relation.ASSOCIATION:
			relatedClassInfos = classInfo.getAssociations();
			break;
		}
		if (classInfo.isLoaded() && relatedClassInfos.length == 0)
		{
			return;
		}
		switch (relationType)
		{
		case Relation.SUPER_TYPE:
			setSuperTypesIndication(areRelationsVisible);
			break;
		case Relation.SUB_TYPE:
			setSubTypesIndication(areRelationsVisible);
			break;
		case Relation.COMPOSITION:
			setCompositionsIndication(areRelationsVisible);
			break;
		case Relation.ASSOCIATION:
			setAssociationsIndication(areRelationsVisible);
			break;
		}
		if (!classInfo.isLoaded())
		{
			if (!areRelationsVisible)
			{
				return;
			}
			new Thread()
			{
				public void run()
				{
					load();
					SwingUtilities.invokeLater(new Runnable()
					{
						public void run()
						{
							switch (relationType)
							{
							case Relation.SUPER_TYPE:
								if (areSuperTypesVisible())
								{
									setRelationsVisible(true, relationType);
								}
								break;
							case Relation.SUB_TYPE:
								if (areSubTypesVisible())
								{
									setRelationsVisible(true, relationType);
								}
								break;
							case Relation.COMPOSITION:
								if (areCompositionsVisible())
								{
									setRelationsVisible(true, relationType);
								}
								break;
							case Relation.ASSOCIATION:
								if (areAssociationsVisible())
								{
									setRelationsVisible(true, relationType);
								}
								break;
							}
						}
					});
				}
			}.start();
			return;
		}
		if (classPane.isClassComponentPresent(classInfo))
		{
			classPane.setRelationsVisible(this, relatedClassInfos, areRelationsVisible,
					relationType);
		}
	}

	public void openPackage(PackageInfo packageInfo)
	{
		try
		{
			classInfo.getClassProcessor().openPackage(packageInfo);
		}
		catch (Exception e)
		{
			e.printStackTrace();
		}
	}

	public void openClass(ClassInfo classInfo)
	{
		try
		{
			classInfo.getClassProcessor().openClass(classInfo);
		}
		catch (Exception e)
		{
			e.printStackTrace();
		}
	}

	public void openMethod(MethodInfo methodInfo)
	{
		try
		{
			classInfo.getClassProcessor().openMethod(methodInfo);
		}
		catch (Exception e)
		{
			e.printStackTrace();
		}
	}

	public void openField(FieldInfo fieldInfo)
	{
		try
		{
			classInfo.getClassProcessor().openField(fieldInfo);
		}
		catch (Exception e)
		{
			e.printStackTrace();
		}
	}

}
