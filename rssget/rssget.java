import java.io.IOException;
import javax.xml.parsers.DocumentBuilder;
import javax.xml.parsers.DocumentBuilderFactory;
import javax.xml.parsers.ParserConfigurationException;
import org.w3c.dom.Document;
import org.w3c.dom.Element;
import org.w3c.dom.NodeList;
import org.xml.sax.SAXException;

public class rssget {

	public static void main(String[] args) {
		String path = "https://news.yahoo.co.jp/rss/topics/top-picks.xml";
		parseXML(path);
	}

	public static void parseXML(String path) {
		try {
			DocumentBuilderFactory	factory = DocumentBuilderFactory.newInstance();
			DocumentBuilder	builder = factory.newDocumentBuilder();
			Document	document = builder.parse(path);
			Element		root = document.getDocumentElement();
			NodeList	channel = root.getElementsByTagName("channel");
			NodeList	title = ((Element)channel.item(0)).getElementsByTagName("title");
			System.out.println("Title: " + title.item(0).getFirstChild().getNodeValue() + "\n");
			NodeList	item_list = root.getElementsByTagName("item");
			for (int i = 0; i < item_list.getLength(); i++) {
				Element	element = (Element)item_list.item(i);
				NodeList item_title = element.getElementsByTagName("title");
				NodeList item_link = element.getElementsByTagName("link");
				System.out.println("  Item: " + item_title.item(0).getFirstChild().getNodeValue());
				System.out.println("  Link: " + item_link.item(0).getFirstChild().getNodeValue() + "\n");
			}
		} catch (IOException e) {
			System.out.println("IOException");
			e.printStackTrace();
		} catch (ParserConfigurationException e) {
			System.out.println("ParserConfigurationException");
			e.printStackTrace();			
		} catch (SAXException e) {
			System.out.println("SAXExceptionn");
			e.printStackTrace();			
		}
		return;
	}
}

