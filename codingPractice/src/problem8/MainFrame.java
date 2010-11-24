package problem8;

import java.awt.BorderLayout;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.JButton;
import javax.swing.JFrame;
import javax.swing.JTabbedPane;
import javax.swing.JToolBar;

public class MainFrame extends JFrame
{
	public MainFrame() {
		setTitle("Book Collector");
		setSize(800, 600);
		setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		// setLayout(new BorderLayout());

		jtp = new JTabbedPane();
		getContentPane().add(jtp, BorderLayout.SOUTH);

		bl = new BookList(jtp);
		getContentPane().add(bl, BorderLayout.CENTER);

		JToolBar tb = new JToolBar();
		tb.setFloatable(false);
		newBtn = new JButton("New");
		delBtn = new JButton("Delete");
		tb.add(newBtn);
		tb.add(delBtn);
		
		newBtn.addActionListener(new ActionListener()
		{
			
			@Override
			public void actionPerformed(ActionEvent e)
			{
				// TODO Auto-generated method stub
				Book b = new Book();
				bl.addBook(b);
				jtp.add("New Book", new BookInfoShower(b, bl));
				jtp.setSelectedIndex(jtp.getTabCount() - 1);
			}
		});
		
		
		delBtn.addActionListener(new ActionListener()
		{
			
			@Override
			public void actionPerformed(ActionEvent e)
			{
				// TODO Auto-generated method stub
				System.err.println("Not implement");
			}
		});
		
		getContentPane().add(tb, BorderLayout.NORTH);

		Book b = new Book();
		b.setISBN("12323412");
		b.setName("APUE");
		b.setAuthor("Stevens");
		bl.addBook(b);
		for (int i = 0; i < 10; ++i) {
			b = new Book();
			b.setISBN(String.valueOf(i + 21345243));
			b.setName("UNP" + i * i);
			b.setAuthor("Stevens" + i);
			bl.addBook(b);
		}
	}

	private JTabbedPane jtp;
	private JButton newBtn, delBtn;
	private BookList bl;
}
