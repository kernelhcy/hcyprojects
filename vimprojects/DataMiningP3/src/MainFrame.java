import arff.ARFFParser;
import arff.Arff;
import java.awt.BorderLayout;
import java.awt.Cursor;
import java.awt.event.ActionEvent;
import java.io.File;
import java.io.IOException;
import javax.swing.*;

import java.awt.Dimension;
import java.awt.FlowLayout;
import java.awt.Font;
import java.awt.event.ActionListener;
import java.awt.event.MouseAdapter;
import java.awt.event.MouseEvent;
import kmeans.KMeans;

public class MainFrame extends JFrame
{
	public MainFrame()
	{
		setTitle("KMeans");
		setDefaultCloseOperation(EXIT_ON_CLOSE);
		setSize(new Dimension(800, 700));
		getContentPane().setLayout(new BorderLayout());

		JPanel p1= new JPanel(new BorderLayout());
		fileSelected = new JTextField();
		fileSelected.setEditable(false);
		fileSelected.setCursor(new Cursor(Cursor.TEXT_CURSOR));
		p1.add(fileSelected, BorderLayout.CENTER);
		JButton fileBtn = new JButton("select");
		p1.add(fileBtn, BorderLayout.EAST);
		p1.setBorder(BorderFactory
				.createTitledBorder("Select *.arff file"));
		add(p1, BorderLayout.NORTH);

		resultArea = new JTextArea();
		resultArea.setTabSize(8);
		resultArea.setFont(new Font("Courier New", Font.PLAIN, 15));
		JScrollPane jsp = new JScrollPane(resultArea);
		jsp.setHorizontalScrollBarPolicy(
			JScrollPane.HORIZONTAL_SCROLLBAR_ALWAYS);
		jsp.setVerticalScrollBarPolicy(
			JScrollPane.VERTICAL_SCROLLBAR_ALWAYS);
		jsp.setBorder(BorderFactory.createTitledBorder("Result"));
		add(jsp, BorderLayout.CENTER);

		JPanel p2 = new JPanel();
		p2.setLayout(new FlowLayout());
		runBtn = new JButton("Run");
		runBtn.setEnabled(false);
		JButton clrBtn = new JButton("Clear");
		kJTF = new JTextField("2");
		kJTF.setColumns(5);
		seedJTF = new JTextField("100");
		seedJTF.setColumns(5);
		p2.add(new JLabel("K:"));
		p2.add(kJTF);
		p2.add(new JLabel("seed:"));
		p2.add(seedJTF);
		p2.add(runBtn);
		p2.add(clrBtn);
		add(p2, BorderLayout.SOUTH);
		runBtn.addActionListener(new ActionListener() {

			public void actionPerformed(ActionEvent e)
			{
				int k = Integer.valueOf(kJTF.getText());
				int seed = Integer.valueOf(seedJTF.getText());
				Arff arff = ARFFParser.parse(file);
				KMeans km = new KMeans(arff, k, seed);
				try {
					km.run();
				}
				catch (IOException ex) {}
				resultArea.setText(km.getResult());
			}
		});
		clrBtn.addActionListener(new ActionListener() {

			public void actionPerformed(ActionEvent e)
			{
				fileSelected.setText("");
				resultArea.setText("");
				runBtn.setEnabled(false);
			}
		});

		FileSelectActionListener fsa = new FileSelectActionListener();
		fileSelected.addMouseListener(fsa);
		fileBtn.addActionListener(fsa);
	}

	/**
	 * Select file.
	 * ѡ���ļ��ļ����ࡣ
	 */
	private class FileSelectActionListener extends MouseAdapter
			implements ActionListener
	{

		public void actionPerformed(ActionEvent e)
		{
			selectFile();
		}

		@Override
		public void mousePressed(MouseEvent e) {
			selectFile();
		}

		private void selectFile()
		{
			JFileChooser jfc = new JFileChooser();
			jfc.setCurrentDirectory(new File("/home/hcy/Desktop/"
				+ "homeworks/DataMining/project3/data/"));
			jfc.setFileSelectionMode(JFileChooser.FILES_ONLY);
			jfc.addChoosableFileFilter(new ArffFileFilter());
			int rv = jfc.showOpenDialog(me);
			if(rv == JFileChooser.APPROVE_OPTION){
				file = jfc.getSelectedFile();
				fileSelected.setText(file.getPath());
				runBtn.setEnabled(true);
			}
		}
		
	}

	/**
	 * Arff file filter
	 * ���ڹ���ѡ����ļ���
	 */
	private class ArffFileFilter extends javax.swing.filechooser.FileFilter
	{

		public boolean accept(File pathname)
		{
			if(pathname.isDirectory() && !pathname.isHidden()){
				return true;
			}
			String name = pathname.getName();
			if(!name.endsWith(".arff")){
				return false;
			}
			return true;
		}

		@Override
		public String getDescription()
		{
			return "*.arff/Arff files";
		}
	}

	private JTextField fileSelected;
	private JTextArea resultArea;
	private JButton runBtn;
	private JFrame me = this;
	private File file;
	private JTextField kJTF, seedJTF;
}
