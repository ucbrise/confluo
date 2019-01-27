package confluo;

import java.io.IOException;
import java.io.InputStream;
import java.net.URL;
import java.util.Map;
import java.util.Properties;
import java.util.Set;

public class PropertiesParser {

    /**
     *  parse unique domain properties
     *  @return  filtered properties by domain
     **/
    public static Properties parse(String domain, String file) throws IOException{
        URL url= PropertiesParser.class.getClassLoader().getResource(file);
        Properties pro=new Properties();
        Properties filtered=new Properties();
        InputStream inputStream = url.openStream();
        pro.load(inputStream);
        Set<Map.Entry<Object,Object>> entries=pro.entrySet();
        String domainKey;
        for(Map.Entry<Object,Object> entry:entries){
            domainKey=(String)entry.getKey();
            if(domainKey.startsWith(domain)){
                filtered.put(domainKey.substring(domain.length()+1),entry.getValue());
            }
        }
        return filtered;
    }
}
