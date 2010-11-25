package problem8;

import java.awt.BorderLayout;
import java.awt.Component;
import java.awt.event.ActionEvent;
import java.awt.event.ActionListener;

import javax.swing.JButton;
import javax.swing.JComboBox;
import javax.swing.JFrame;
import javax.swing.JLabel;
import javax.swing.JTabbedPane;
import javax.swing.JTextField;
import javax.swing.JToolBar;
import javax.swing.event.DocumentEvent;
import javax.swing.event.DocumentListener;

import problem8.eventagent.Event;
import problem8.eventagent.EventAgent;
import problem8.eventagent.EventResponser;

public class MainFrame extends JFrame
{
	public MainFrame() {
		setTitle("Book Collector");
		setSize(800, 600);
		setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		// setLayout(new BorderLayout());

		jtp = new JTabbedPane();
		getContentPane().add(jtp, BorderLayout.SOUTH);
		
		//JTABBEDPANE_REMOVEALL event
		EventAgent.registerEvent(new EventResponser()
		{
			
			@Override
			public String getName()
			{
				// TODO Auto-generated method stub
				return "JTABBEDPANE_REMOVEALL";
			}
			
			@Override
			public void doAction(Object[] args, int argc)
			{
				// TODO Auto-generated method stub
				jtp.removeAll();
			}
		});
		
		//JTABBEDPANE_ADDTAB event
		EventAgent.registerEvent(new EventResponser()
		{
			
			@Override
			public String getName()
			{
				// TODO Auto-generated method stub
				return "JTABBEDPANE_ADDTAB";
			}
			
			/**
			 * args[0] : the name of the tab
			 * args[1] : the component of the tab
			 */
			@Override
			public void doAction(Object[] args, int argc)
			{
				// TODO Auto-generated method stub
				//we need the tab name and the component.
				if(argc < 2){
					return;
				}
				
				jtp.addTab((String)args[0]
				                  , (Component)args[1]);
			}
		});
		
		//JTABBEDPANE_UPDATETABNAME event
		EventAgent.registerEvent(new EventResponser()
		{
			
			@Override
			public String getName()
			{
				// TODO Auto-generated method stub
				return "JTABBEDPANE_UPDATETABNAME";
			}
			
			/**
			 * args[0] : the index of the tab
			 * args[1] : the new name
			 */
			@Override
			public void doAction(Object[] args, int argc)
			{
				// TODO Auto-generated method stub
				if(argc < 2){
					//we need tab index and the new
					//name.
					return;
				}
				jtp.setTitleAt((Integer)args[0]
				                     , (String)args[1]);
			}
		});
		
		bl = new BookList();
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
				BookInfoShower bis = new BookInfoShower(b);
				bl.addBook(b);
				
				jtp.add("New Book", bis);
				jtp.setSelectedIndex(jtp.getTabCount() - 1);
				bis.setIndex(jtp.getTabCount() -1);
				
				selRng.clearArg();
				selRng.addArg(bl.getBookNum() - 1);
				selRng.addArg(bl.getBookNum() - 1);
				EventAgent.dispatchEvent(selRng);
			}
		});
		
		delBtn.addActionListener(new ActionListener()
		{
			
			@Override
			public void actionPerformed(ActionEvent e)
			{
				// TODO Auto-generated method stub
				EventAgent.dispatchEvent(delSel);
			}
		});
		
		tb.add(searchLabel);
		searchJf = new JTextField();
		searchCb = new JComboBox();
		searchCb.addItem("Style");
		searchCb.addItem("Author");
		searchCb.addItem("Publisher");
		searchCb.addItem("Borrower");
		searchCb.addItem("Name");
		tb.add(searchCb);
		tb.add(searchJf);
		
		searchJf.getDocument().addDocumentListener(new DocumentListener()
		{
			
			@Override
			public void removeUpdate(DocumentEvent e)
			{
				// TODO Auto-generated method stub
				doAction();
			}
			
			@Override
			public void insertUpdate(DocumentEvent e)
			{
				// TODO Auto-generated method stub
				doAction();
			}
			
			@Override
			public void changedUpdate(DocumentEvent e)
			{
				// TODO Auto-generated method stub
				doAction();
			}
			
			private void doAction(){
				filter.clearArg();
				filter.addArg(searchJf.getText());
				filter.addArg(searchCb.getSelectedItem());
				EventAgent.dispatchEvent(filter);
			}
			
			private Event filter = new Event("BOOKLIST_ROWFILTER");
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
	private JLabel searchLabel = new JLabel("Search:");
	private JTextField searchJf;
	private JComboBox searchCb;
	private BookList bl;
	
	private Event delSel = new Event("BOOKLIST_DELSELECTED");
	private Event selRng = new Event("BOOKLIST_SELECTRANGE");
}
