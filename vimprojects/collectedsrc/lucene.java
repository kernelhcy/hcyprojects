package com.diyi.util;
import java.io.File;
import java.io.IOException;
import java.io.StringReader;
import java.util.ArrayList;
import java.util.List;

import org.apache.lucene.analysis.Analyzer;
import org.apache.lucene.analysis.TokenStream;
import org.apache.lucene.analysis.standard.StandardAnalyzer;
import org.apache.lucene.document.Document;
import org.apache.lucene.document.Field;
import org.apache.lucene.index.IndexWriter;
import org.apache.lucene.index.Term;
import org.apache.lucene.queryParser.MultiFieldQueryParser;
import org.apache.lucene.search.BooleanClause;
import org.apache.lucene.search.BooleanQuery;
import org.apache.lucene.search.FuzzyQuery;
import org.apache.lucene.search.Hits;
import org.apache.lucene.search.IndexSearcher;
import org.apache.lucene.search.MultiSearcher;
import org.apache.lucene.search.PrefixQuery;
import org.apache.lucene.search.Query;
import org.apache.lucene.search.RangeQuery;
import org.apache.lucene.search.Sort;
import org.apache.lucene.search.TermQuery;
import org.apache.lucene.search.WildcardQuery;
import org.apache.lucene.search.highlight.Highlighter;
import org.apache.lucene.search.highlight.QueryScorer;
import org.apache.lucene.search.highlight.SimpleFragmenter;
import org.apache.lucene.search.highlight.SimpleHTMLFormatter;
import org.mira.lucene.analysis.IK_CAnalyzer;

import com.diyi.core.web.BaseAction;
public class LuceneTest extends BaseAction{
    
        private static final long serialVersionUID = 1L;
    /*
     * lucene全功能，几乎包含了大部分查询综合起来。，
     */
        static String path="e:\\Lucene测试\\";
        static String ArticleTitle="ArticleTitle";
        static String ArticleText="ArticleText";
        static String ArticleTime="ArticleTime";
        public static void index() throws Exception {
            /* 创建索引初始化，执行这些语句将创建或清空d:\\save\\目录下所有索引 */
            File file=new File(path);
            if(file.isDirectory())
            {
                file.delete();
            }
            IK_CAnalyzer ikAnalyzer=new IK_CAnalyzer();
            IndexWriter writer1 = new IndexWriter(path,ikAnalyzer, true);
             writer1.close();
//              IndexReader indexReader=IndexReader.open(path);
//              indexReader.deleteDocument(1);
            /*
             * 往创建的初始化索引中添加索引内容，StandardAnalyzer表示用lucene自带的标准分词机制，
             * false表示不覆盖原来该目录的索引，细心的读者可能已经发现， 这句话和上面的那句就这个false不一样
             */
            IndexWriter writer2 = new IndexWriter(path,
                    ikAnalyzer, false);
            /* 创建一份文件 */
            Document doc1 = new Document();
            /*
             * 创建一个域ArticleTitle，并往这个域里面添加内容 "Field.Store.YES"表示域里面的内容将被存储到索引
             * "Field.Index.TOKENIZED"表示域里面的内容将被索引，以便用来搜索
             *  Lucene给文档的字段设定三个布尔变量: 索引(indexed), 存储(stored), 切词(tokenized) ,
             */
            Field field1 = new Field(ArticleTitle, "上海2010年世博会,hot,GOOGLE和Yahoo 赞助,test,中华人民共和国万岁", Field.Store.YES,
                    Field.Index.TOKENIZED);
            /* 往文件里添加这个域 */
            doc1.add(field1);
            /* 同理：创建另外一个域ArticleText，并往这个域里面添加内容 */
            Field field2 = new Field(ArticleText, "这是一届创造绿色环境,点燃激情,影响深远的博览会....god..Hotmail,text,foam,OpenOffica",
                    Field.Store.YES, Field.Index.TOKENIZED);
            doc1.add(field2);
           
            Field field3 = new Field(ArticleTime, "2009",
                    Field.Store.YES, Field.Index.TOKENIZED);
            doc1.add(field3);
            // 在这里还可以添加其他域
            /* 添加这份文件到索引 */
            writer2.addDocument(doc1);

            /* 同理：创建第二份文件 */
            Document doc2 = new Document();
            field1 = new Field(ArticleTitle, "中国获得全球赞誉,世界人民大团结万岁,text", Field.Store.YES,
                    Field.Index.TOKENIZED);
            doc2.add(field1);
            field2 = new Field(ArticleText, "中国上海世博馆雄踞东方,傲视全球........,roams,OpenOffice",
                    Field.Store.YES, Field.Index.TOKENIZED);
            doc2.add(field2);
            field3 = new Field(ArticleTime, "2010",
                    Field.Store.YES, Field.Index.TOKENIZED);
            doc2.add(field3);
          /*
           *
           *
           */
            writer2.addDocument(doc2);

            // 在这里可以添加其他文件
            //writer2.optimize();
            /* 关闭 */
            writer2.close();
        }

        public  String  searchIndex() throws Exception {
             LuceneTest.index();
             String keywords=getRequest().getParameter("serchString");
            /* 创建一个搜索，搜索刚才创建的目录下的索引 */
            IndexSearcher indexSearcher = new IndexSearcher(path);
            /* 在这里我们只需要搜索一个目录 */
            IndexSearcher indexSearchers[] = { indexSearcher };
            /* 我们需要搜索两个域ArticleTitle, ArticleText里面的内容 */
            String[] fields = {ArticleTitle,
                    ArticleText ,ArticleTime};
            /* 下面这个表示要同时搜索这两个域，而且只要一个域里面有满足我们搜索的内容就行
            BooleanClause.Occur[]数组,它表示多个条件之间的关系,BooleanClause.Occur.MUST表示and,
            BooleanClause.Occur.MUST_NOT表示not,BooleanClause.Occur.SHOULD表示or.
            1、MUST和MUST表示“与”的关系，即“并集”。
            2、MUST和MUST_NOT前者包含后者不包含。
            3、MUST_NOT和MUST_NOT没意义
            4、SHOULD与MUST表示MUST，SHOULD失去意义；
            5、SHOUlD与MUST_NOT相当于MUST与MUST_NOT。
            6、SHOULD与SHOULD表示“或”的概念*/
            BooleanClause.Occur[] clauses = { BooleanClause.Occur.SHOULD,
                    BooleanClause.Occur.SHOULD,BooleanClause.Occur.SHOULD};
            /*
             * MultiFieldQueryParser表示多个域解析，
             * 同时可以解析含空格的字符串，如果我们搜索"上海 中国"
             */
            Analyzer analyzer=new  IK_CAnalyzer();
            Query multiFieldQuery = MultiFieldQueryParser.parse(keywords, fields, clauses,
                    analyzer);
            Query termQuery= new TermQuery(new Term(ArticleTitle, keywords));//词语搜索,完全匹配,搜索具体的域
           
            Query wildqQuery=new WildcardQuery(new Term(ArticleTitle,keywords));//通配符查询
           
            Query prefixQuery=new PrefixQuery(new Term(ArticleText,keywords));//字段前缀搜索
           
            Query fuzzyQuery=new FuzzyQuery(new Term(ArticleText,keywords));//相似度查询,模糊查询比如OpenOffica，OpenOffice
             
            Query rangQuery=new RangeQuery(new Term(ArticleTime,keywords), new Term(ArticleTime,keywords), true);//数字范围查询
            /* Multisearcher表示多目录搜索，在这里我们只有一个目录 */
            MultiSearcher searcher = new MultiSearcher(indexSearchers);
            //多条件搜索
            BooleanQuery multiQuery=new BooleanQuery();
           
            multiQuery.add(wildqQuery, BooleanClause.Occur.SHOULD);
            multiQuery.add(multiFieldQuery, BooleanClause.Occur.SHOULD);
            multiQuery.add(termQuery, BooleanClause.Occur.SHOULD);
            multiQuery.add(prefixQuery, BooleanClause.Occur.SHOULD);
            multiQuery.add(fuzzyQuery, BooleanClause.Occur.SHOULD);
            multiQuery.add(rangQuery, BooleanClause.Occur.SHOULD);
           
            Sort sort=new Sort(ArticleTime);//排序
            /* 开始搜索 */
            Hits h = searcher.search(multiQuery,sort);
            String highTitle="";
            String highText="";
            List<String> list=new ArrayList<String>();
             /* 把搜索出来的所有文件打印出来 */
            for (int i = 0; i < h.length(); i++) {
                 //打印出文件里面ArticleTitle域里面的内容
                String title=h.doc(i).get(ArticleTitle);
                // 打印出文件里面ArticleText域里面的内容 
                String text=h.doc(i).get(ArticleText);
                SimpleHTMLFormatter format=new SimpleHTMLFormatter("<b><font color='red'>","</font></b>");
                Highlighter light=new Highlighter(format, new QueryScorer(multiQuery));//高亮
                light.setTextFragmenter(new SimpleFragmenter(200));
                if(title!=null)
                {
                TokenStream stream=analyzer.tokenStream(ArticleTitle, new StringReader(title));
                highTitle=light.getBestFragment(stream, title);
                System.out.println(highTitle);
                }
                if(text!=null)
                {
                TokenStream streamText=analyzer.tokenStream(ArticleText, new StringReader(text));
                highText=light.getBestFragment(streamText, text);
                System.out.println(highText);
                }
              //为了在页面好遍历，把它放入集合中
                if(highTitle!=null)
                {
                    list.add("标题中包含关键字："+highTitle+"<br/>");
                }
                if(highText!=null)
                {
                    list.add("内容中包含关键字："+highText+"<br/>");
                }
               
               
            }
            getRequest().setAttribute("list",list);
            /* 关闭 */
            searcher.close();
            return SUCCESS;
        }
        public String goToSearch()
        {
            return SUCCESS;
        }
          
         // 通配符搜索 WildcardQuery    
         // 通配符包括’?’匹配一个任意字符和’*’匹配零个或多个任意字符，例如你搜索’use*’，你可能找到’user’或者’uses’：    
         public static void wildcardSearcher() throws Exception{ 
               index();
                 IndexSearcher searcher = new IndexSearcher(path); 
                  
                 // 与正则一样,*代表0个或多个字母,?代表0个或一个字母 
                 // WildcardQuery与QueryParser不同的是:WildcardQuery的前缀可以为*,而QueryParser不行 
                 Query query = new WildcardQuery(new Term(ArticleText,"te*")); 
                  
                 Hits hits = searcher.search(query); 
                  
                 printResult(hits); 
                  
                 searcher.close(); 
         } 
         public static void printResult(Hits hits) throws IOException{ 
             for(int i = 0; i < hits.length(); i++){ 
                 Document d = hits.doc(i);
                 String title=d.get(ArticleTitle);
                 String text=d.get(ArticleText);
                 String time=d.get(ArticleTime);
                 if(title!=null)
                 {
                     System.out.println(title);
                 }
                 if(text!=null)
                 {
                     System.out.println(text);
                 }
                 if(time!=null)
                 {
                     System.out.println(time);
                 }
              
             } 
         } 
        public static void main(String[] args) throws Exception {
            LuceneTest test=new LuceneTest();
            test.wildcardSearcher();
           LuceneTest .searchIndex();
        }
    }
</span>


