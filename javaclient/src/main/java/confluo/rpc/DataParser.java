package confluo.rpc;

import java.nio.ByteBuffer;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;

class DataParser {

  interface Parser {
    void parseFromString(ByteBuffer out, String data, int size);
    String parseToString(ByteBuffer in, int size);
  }

  static class NoneParser implements Parser {
    @Override
    public void parseFromString(ByteBuffer out, String data, int size) {
      throw new UnsupportedOperationException();
    }

    @Override
    public String parseToString(ByteBuffer in, int size) {
      throw new UnsupportedOperationException();
    }
  }

  static class BooleanParser implements Parser {
    @Override
    public void parseFromString(ByteBuffer out, String data, int size) {
      out.put((byte) (Boolean.parseBoolean(data) ? 1 : 0));
    }

    @Override
    public String parseToString(ByteBuffer in, int size) {
      return String.valueOf(in.get(0) == 1);
    }
  }

  static class CharParser implements Parser {
    @Override
    public void parseFromString(ByteBuffer out, String data, int size) {
      out.put((byte) data.charAt(0));
    }

    @Override
    public String parseToString(ByteBuffer in, int size) {
      return String.format("'%c'", in.get());
    }
  }

  static class ShortParser implements Parser {
    @Override
    public void parseFromString(ByteBuffer out, String data, int size) {
      out.putShort(Short.parseShort(data));
    }

    @Override
    public String parseToString(ByteBuffer in, int size) {
      return String.valueOf(in.getShort());
    }
  }

  static class IntParser implements Parser {
    @Override
    public void parseFromString(ByteBuffer out, String data, int size) {
      out.putInt(Integer.parseInt(data));
    }

    @Override
    public String parseToString(ByteBuffer in, int size) {
      return String.valueOf(in.getInt());
    }
  }

  static class LongParser implements Parser {
    @Override
    public void parseFromString(ByteBuffer out, String data, int size) {
      out.putLong(Long.parseLong(data));
    }

    @Override
    public String parseToString(ByteBuffer in, int size) {
      return String.valueOf(in.getLong());
    }
  }

  static class FloatParser implements Parser {
    @Override
    public void parseFromString(ByteBuffer out, String data, int size) {
      out.putFloat(Float.parseFloat(data));
    }

    @Override
    public String parseToString(ByteBuffer in, int size) {
      return String.valueOf(in.getFloat());
    }
  }

  static class DoubleParser implements Parser {
    @Override
    public void parseFromString(ByteBuffer out, String data, int size) {
      out.putDouble(Double.parseDouble(data));
    }

    @Override
    public String parseToString(ByteBuffer in, int size) {
      return String.valueOf(in.getDouble());
    }
  }

  static class StringParser implements Parser {

    @Override
    public void parseFromString(ByteBuffer out, String data, int size) {
      out.put(StandardCharsets.UTF_8.encode(data));
      out.position(out.position() + (size - data.length()));
    }

    @Override
    public String parseToString(ByteBuffer in, int size) {
      String value = StandardCharsets.UTF_8.decode(in).toString();
      int endOff = value.indexOf('\0');
      if (endOff != -1)
        return value.substring(0, endOff);
      return value;
    }
  }

  private static ArrayList<Parser> PARSERS;
  static {
    PARSERS = new ArrayList<>();
    PARSERS.add(new NoneParser()); // 0
    PARSERS.add(new BooleanParser()); // 1
    PARSERS.add(new CharParser()); // 2
    PARSERS.add(new CharParser()); // 3
    PARSERS.add(new ShortParser()); // 4
    PARSERS.add(new ShortParser()); // 5
    PARSERS.add(new IntParser()); // 6
    PARSERS.add(new IntParser()); // 7
    PARSERS.add(new LongParser()); // 8
    PARSERS.add(new LongParser()); // 9
    PARSERS.add(new FloatParser()); // 10
    PARSERS.add(new DoubleParser()); // 11
    PARSERS.add(new StringParser()); // 12
  }

  static void parseFromString(DataType type, ByteBuffer out, String data) {
    PARSERS.get(type.typeId.getValue()).parseFromString(out, data, type.size);
  }

  static String parseToString(DataType type, ByteBuffer in) {
    String value = PARSERS.get(type.typeId.getValue()).parseToString(in, type.size);
    in.rewind();
    return value;
  }
}
