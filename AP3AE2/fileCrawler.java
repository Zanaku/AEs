import java.io.File;
import java.util.Iterator;
import java.util.concurrent.ConcurrentLinkedQueue;
import java.util.concurrent.ConcurrentSkipListSet;
import java.util.regex.*;

public class fileCrawler {
	
	/* The Main thread, which reads directories into the Work Queue, and harvest the 'Another Structure'
	 * Must wait for worker thread to finish before it can harvest.
	 */
	public static class MainThread implements Runnable {

		public void run() {
			
		}
		
		
	}
	
	/* Worker thread, of which there may be many. This reads directories from the Work Queue and processes for harvesting.
	 * Must be able to determine when no more directories shall be added to the Work Queue.
	 */
	public static class WorkerThread implements Runnable{

		public void run() {
			
		}
		
	}
	
	//this method is taken from the provided Directory Tree class, and modified slightly.
	public void processDirectory(String name,ConcurrentLinkedQueue<String> ll) {
		try {
			File file = new File(name); // create a File object
			if (file.isDirectory()) { // a directory - could be symlink
				String entries[] = file.list();
				if (entries != null) { // not a symlink
					//System.out.println("Dir: "+name);// print out the name
					ll.add(name);
					for (String entry : entries) {
						if (entry.compareTo(".") == 0)
							continue;
						if (entry.compareTo("..") == 0)
							continue;
						processDirectory(name + "/" + entry,ll);
					}
				}
			}
/*			else {
				ll.add(name); //add to linked list
				System.out.println("File: "+name);// print out the name
			}*/

		} catch (Exception e) {
			System.err.println("Error processing " + name + ": " + e);
		}
	}

	/* taken from provided Regex file
	 * converts bash pattern to Regex string
	 * due to vagaries of parameter expansion on Windows, this code will
	 * strip off leading and trailing apostrophes (') from `str'
	 *
	 * the RegEx string is generated as follows
	 * '^' is put at the beginning of the string
	 * '*' is converted to ".*"
	 * '.' is converted to "\."
	 * '?' is converted to "."
	 * '$' is put at the end of the string
	 * 
	 */
	public static String cvtPattern(String str) {
		StringBuilder pat = new StringBuilder();
		int start, length;

		pat.append('^');
		if (str.charAt(0) == '\'') {	// double quoting on Windows
			start = 1;
			length = str.length() - 1;
		} else {
			start = 0;
			length = str.length();
		}
		for (int i = start; i < length; i++) {
			switch(str.charAt(i)) {
			case '*': pat.append('.'); pat.append('*'); break;
			case '.': pat.append('\\'); pat.append('.'); break;
			case '?': pat.append('.'); break;
			default:  pat.append(str.charAt(i)); break;
			}
		}
		pat.append('$');
		return new String(pat);
	}
	
	public static void main(String[] args) {
		
		/* Read environment variable CRAWLER_THREADS
		 * if not defined, set # of threads to 2. Otherwise use provided value.
    	 */
		int threadNum = 0;
		String cT = System.getenv("CRAWLER_THREADS");
		if(cT!=null){
			threadNum = Integer.parseInt(cT);
		}
		else threadNum = 2;
		
		fileCrawler fC = new fileCrawler();
		String pattern;
		if(args.length<1){
			System.err.println("Usage: ./fileCrawler pattern [dir] ...\n");
		}
		/*
		 * convert bash expression to regular expression and compile
		 */
		pattern = fileCrawler.cvtPattern(args[0]);
		// compile RegEx pattern into Pattern object
		Pattern p = Pattern.compile(pattern);
		//create concurrent linked queue, representing the Work Queue and a concurrent skip list set representing 'another structure'.
		ConcurrentLinkedQueue<String> ll = new ConcurrentLinkedQueue<String>();
		ConcurrentSkipListSet<String> ts = new ConcurrentSkipListSet<String>();		
		//add the directories to the ll
		if(args.length==1){
			fC.processDirectory(".", ll);
		}
		else{
			for(int i=1;i<args.length;i++){
				fC.processDirectory(args[i], ll);
			}
		}
		//go through directories, apply regex to files. If match, add to tree set.
		while(!ll.isEmpty()){
			String entry = ll.remove();
			//System.out.println("From tree: "+entry);
			File file = new File(entry);
			for(String s : file.list()){
				File f = new File(entry+"/"+s);
				if(!(f.isDirectory())){
					Matcher m = p.matcher(s);
					if(m.matches()){
						ts.add(entry+"/"+s);
					}
				}
			}

		}
		//iterate over ts, print out entries.
		Iterator<String> it = ts.iterator();
		while(it.hasNext()){
			System.out.println(it.next());
		}
	}

}
