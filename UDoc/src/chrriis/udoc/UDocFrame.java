/*
 * Christopher Deckers (chrriis@nextencia.net)
 * http://www.nextencia.net
 * 
 * See the file "readme.txt" for information on usage and redistribution of
 * this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */
package chrriis.udoc;

import java.awt.BorderLayout;
import java.awt.Container;
import java.awt.Toolkit;

import javax.swing.JFrame;
import javax.swing.SwingUtilities;

import chrriis.udoc.model.processor.ClassProcessor;
import chrriis.udoc.model.processor.ClassProcessorRegistry;
import chrriis.udoc.model.processor.binary.BinaryClassProcessor;
import chrriis.udoc.model.processor.javadoc.JavadocClassProcessor;
import chrriis.udoc.model.processor.source.SourceClassProcessor;
import chrriis.udoc.ui.ClassPane;
import chrriis.udoc.ui.IconManager;

public class UDocFrame
{
	
	public static void main(String[] args)
	{
		System.setProperty("java.net.useSystemProxies", "true");
		Toolkit.getDefaultToolkit().setDynamicLayout(true);
		JFrame frame = new JFrame("UDoc");
		frame.setIconImage(IconManager.getImage("UDoc32x32.png"));
		frame.setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		ClassProcessorRegistry.register(new JavadocClassProcessor());
		ClassProcessorRegistry.register(new BinaryClassProcessor());
		ClassProcessorRegistry.register(new SourceClassProcessor());
		final ClassPane classPane = new ClassPane();
		Container contentPane = frame.getContentPane();
		contentPane.add(classPane, BorderLayout.CENTER);
		frame.setSize(800, 600);
		contentPane.invalidate();
		frame.doLayout();
		frame.setVisible(true);
		if (args.length > 1)
		{
			String processorID = args[0];
			final String classNames = args[1];
			final ClassProcessor classProcessor = ClassProcessorRegistry.getClassProcessor(processorID);
			if (classProcessor != null)
			{
				if (classProcessor instanceof JavadocClassProcessor)
				{
					if (args.length > 2)
					{
						((JavadocClassProcessor) classProcessor).setDocumentationRoot(args[2]);
					}
				}
				else if (classProcessor instanceof BinaryClassProcessor)
				{
					if (args.length > 2)
					{
						((BinaryClassProcessor) classProcessor).setClassPath(args[2]);
					}
				}
				else if (classProcessor instanceof SourceClassProcessor)
				{
					if (args.length > 3)
					{
						SourceClassProcessor sourceClassProcessor = (SourceClassProcessor) classProcessor;
						sourceClassProcessor.setSourcePath(args[2]);
						sourceClassProcessor.setClassPath(args[3]);
					}
				}
				SwingUtilities.invokeLater(new Runnable()
				{
					public void run()
					{
						classPane.setContent(classNames, classProcessor);
					}
				});
			}
		}
	}
	
}
