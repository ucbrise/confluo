/**
 * Autogenerated by Thrift Compiler (0.11.0)
 *
 * DO NOT EDIT UNLESS YOU ARE SURE THAT YOU KNOW WHAT YOU ARE DOING
 *  @generated
 */
package confluo.rpc;

@SuppressWarnings({"cast", "rawtypes", "serial", "unchecked", "unused"})
@javax.annotation.Generated(value = "Autogenerated by Thrift Compiler (0.11.0)", date = "2018-04-02")
public class rpc_iterator_handle implements org.apache.thrift.TBase<rpc_iterator_handle, rpc_iterator_handle._Fields>, java.io.Serializable, Cloneable, Comparable<rpc_iterator_handle> {
  private static final org.apache.thrift.protocol.TStruct STRUCT_DESC = new org.apache.thrift.protocol.TStruct("rpc_iterator_handle");

  private static final org.apache.thrift.protocol.TField DESC_FIELD_DESC = new org.apache.thrift.protocol.TField("desc", org.apache.thrift.protocol.TType.STRUCT, (short)1);
  private static final org.apache.thrift.protocol.TField DATA_FIELD_DESC = new org.apache.thrift.protocol.TField("data", org.apache.thrift.protocol.TType.STRING, (short)2);
  private static final org.apache.thrift.protocol.TField NUM_ENTRIES_FIELD_DESC = new org.apache.thrift.protocol.TField("num_entries", org.apache.thrift.protocol.TType.I32, (short)3);
  private static final org.apache.thrift.protocol.TField HAS_MORE_FIELD_DESC = new org.apache.thrift.protocol.TField("has_more", org.apache.thrift.protocol.TType.BOOL, (short)4);

  private static final org.apache.thrift.scheme.SchemeFactory STANDARD_SCHEME_FACTORY = new rpc_iterator_handleStandardSchemeFactory();
  private static final org.apache.thrift.scheme.SchemeFactory TUPLE_SCHEME_FACTORY = new rpc_iterator_handleTupleSchemeFactory();

  private rpc_iterator_descriptor desc; // required
  private java.nio.ByteBuffer data; // required
  private int num_entries; // required
  private boolean has_more; // required

  /** The set of fields this struct contains, along with convenience methods for finding and manipulating them. */
  public enum _Fields implements org.apache.thrift.TFieldIdEnum {
    DESC((short)1, "desc"),
    DATA((short)2, "data"),
    NUM_ENTRIES((short)3, "num_entries"),
    HAS_MORE((short)4, "has_more");

    private static final java.util.Map<java.lang.String, _Fields> byName = new java.util.HashMap<java.lang.String, _Fields>();

    static {
      for (_Fields field : java.util.EnumSet.allOf(_Fields.class)) {
        byName.put(field.getFieldName(), field);
      }
    }

    /**
     * Find the _Fields constant that matches fieldId, or null if its not found.
     */
    public static _Fields findByThriftId(int fieldId) {
      switch(fieldId) {
        case 1: // DESC
          return DESC;
        case 2: // DATA
          return DATA;
        case 3: // NUM_ENTRIES
          return NUM_ENTRIES;
        case 4: // HAS_MORE
          return HAS_MORE;
        default:
          return null;
      }
    }

    /**
     * Find the _Fields constant that matches fieldId, throwing an exception
     * if it is not found.
     */
    public static _Fields findByThriftIdOrThrow(int fieldId) {
      _Fields fields = findByThriftId(fieldId);
      if (fields == null) throw new java.lang.IllegalArgumentException("Field " + fieldId + " doesn't exist!");
      return fields;
    }

    /**
     * Find the _Fields constant that matches name, or null if its not found.
     */
    public static _Fields findByName(java.lang.String name) {
      return byName.get(name);
    }

    private final short _thriftId;
    private final java.lang.String _fieldName;

    _Fields(short thriftId, java.lang.String fieldName) {
      _thriftId = thriftId;
      _fieldName = fieldName;
    }

    public short getThriftFieldId() {
      return _thriftId;
    }

    public java.lang.String getFieldName() {
      return _fieldName;
    }
  }

  // isset id assignments
  private static final int __NUM_ENTRIES_ISSET_ID = 0;
  private static final int __HAS_MORE_ISSET_ID = 1;
  private byte __isset_bitfield = 0;
  public static final java.util.Map<_Fields, org.apache.thrift.meta_data.FieldMetaData> metaDataMap;
  static {
    java.util.Map<_Fields, org.apache.thrift.meta_data.FieldMetaData> tmpMap = new java.util.EnumMap<_Fields, org.apache.thrift.meta_data.FieldMetaData>(_Fields.class);
    tmpMap.put(_Fields.DESC, new org.apache.thrift.meta_data.FieldMetaData("desc", org.apache.thrift.TFieldRequirementType.REQUIRED, 
        new org.apache.thrift.meta_data.StructMetaData(org.apache.thrift.protocol.TType.STRUCT, rpc_iterator_descriptor.class)));
    tmpMap.put(_Fields.DATA, new org.apache.thrift.meta_data.FieldMetaData("data", org.apache.thrift.TFieldRequirementType.REQUIRED, 
        new org.apache.thrift.meta_data.FieldValueMetaData(org.apache.thrift.protocol.TType.STRING        , true)));
    tmpMap.put(_Fields.NUM_ENTRIES, new org.apache.thrift.meta_data.FieldMetaData("num_entries", org.apache.thrift.TFieldRequirementType.REQUIRED, 
        new org.apache.thrift.meta_data.FieldValueMetaData(org.apache.thrift.protocol.TType.I32)));
    tmpMap.put(_Fields.HAS_MORE, new org.apache.thrift.meta_data.FieldMetaData("has_more", org.apache.thrift.TFieldRequirementType.REQUIRED, 
        new org.apache.thrift.meta_data.FieldValueMetaData(org.apache.thrift.protocol.TType.BOOL)));
    metaDataMap = java.util.Collections.unmodifiableMap(tmpMap);
    org.apache.thrift.meta_data.FieldMetaData.addStructMetaDataMap(rpc_iterator_handle.class, metaDataMap);
  }

  public rpc_iterator_handle() {
  }

  public rpc_iterator_handle(
    rpc_iterator_descriptor desc,
    java.nio.ByteBuffer data,
    int num_entries,
    boolean has_more)
  {
    this();
    this.desc = desc;
    this.data = org.apache.thrift.TBaseHelper.copyBinary(data);
    this.num_entries = num_entries;
    setNumEntriesIsSet(true);
    this.has_more = has_more;
    setHasMoreIsSet(true);
  }

  /**
   * Performs a deep copy on <i>other</i>.
   */
  public rpc_iterator_handle(rpc_iterator_handle other) {
    __isset_bitfield = other.__isset_bitfield;
    if (other.isSetDesc()) {
      this.desc = new rpc_iterator_descriptor(other.desc);
    }
    if (other.isSetData()) {
      this.data = org.apache.thrift.TBaseHelper.copyBinary(other.data);
    }
    this.num_entries = other.num_entries;
    this.has_more = other.has_more;
  }

  public rpc_iterator_handle deepCopy() {
    return new rpc_iterator_handle(this);
  }

  @Override
  public void clear() {
    if (this.desc != null) {
      this.desc.clear();
    }
    this.data = null;
    setNumEntriesIsSet(false);
    this.num_entries = 0;
    setHasMoreIsSet(false);
    this.has_more = false;
  }

  public rpc_iterator_descriptor getDesc() {
    return this.desc;
  }

  public rpc_iterator_handle setDesc(rpc_iterator_descriptor desc) {
    this.desc = desc;
    return this;
  }

  public void unsetDesc() {
    this.desc = null;
  }

  /** Returns true if field desc is set (has been assigned a value) and false otherwise */
  public boolean isSetDesc() {
    return this.desc != null;
  }

  public void setDescIsSet(boolean value) {
    if (!value) {
      this.desc = null;
    }
  }

  public byte[] getData() {
    setData(org.apache.thrift.TBaseHelper.rightSize(data));
    return data == null ? null : data.array();
  }

  public java.nio.ByteBuffer bufferForData() {
    return org.apache.thrift.TBaseHelper.copyBinary(data);
  }

  public rpc_iterator_handle setData(byte[] data) {
    this.data = data == null ? (java.nio.ByteBuffer)null : java.nio.ByteBuffer.wrap(data.clone());
    return this;
  }

  public rpc_iterator_handle setData(java.nio.ByteBuffer data) {
    this.data = org.apache.thrift.TBaseHelper.copyBinary(data);
    return this;
  }

  public void unsetData() {
    this.data = null;
  }

  /** Returns true if field data is set (has been assigned a value) and false otherwise */
  public boolean isSetData() {
    return this.data != null;
  }

  public void setDataIsSet(boolean value) {
    if (!value) {
      this.data = null;
    }
  }

  public int getNumEntries() {
    return this.num_entries;
  }

  public rpc_iterator_handle setNumEntries(int num_entries) {
    this.num_entries = num_entries;
    setNumEntriesIsSet(true);
    return this;
  }

  public void unsetNumEntries() {
    __isset_bitfield = org.apache.thrift.EncodingUtils.clearBit(__isset_bitfield, __NUM_ENTRIES_ISSET_ID);
  }

  /** Returns true if field num_entries is set (has been assigned a value) and false otherwise */
  public boolean isSetNumEntries() {
    return org.apache.thrift.EncodingUtils.testBit(__isset_bitfield, __NUM_ENTRIES_ISSET_ID);
  }

  public void setNumEntriesIsSet(boolean value) {
    __isset_bitfield = org.apache.thrift.EncodingUtils.setBit(__isset_bitfield, __NUM_ENTRIES_ISSET_ID, value);
  }

  public boolean isHasMore() {
    return this.has_more;
  }

  public rpc_iterator_handle setHasMore(boolean has_more) {
    this.has_more = has_more;
    setHasMoreIsSet(true);
    return this;
  }

  public void unsetHasMore() {
    __isset_bitfield = org.apache.thrift.EncodingUtils.clearBit(__isset_bitfield, __HAS_MORE_ISSET_ID);
  }

  /** Returns true if field has_more is set (has been assigned a value) and false otherwise */
  public boolean isSetHasMore() {
    return org.apache.thrift.EncodingUtils.testBit(__isset_bitfield, __HAS_MORE_ISSET_ID);
  }

  public void setHasMoreIsSet(boolean value) {
    __isset_bitfield = org.apache.thrift.EncodingUtils.setBit(__isset_bitfield, __HAS_MORE_ISSET_ID, value);
  }

  public void setFieldValue(_Fields field, java.lang.Object value) {
    switch (field) {
    case DESC:
      if (value == null) {
        unsetDesc();
      } else {
        setDesc((rpc_iterator_descriptor)value);
      }
      break;

    case DATA:
      if (value == null) {
        unsetData();
      } else {
        if (value instanceof byte[]) {
          setData((byte[])value);
        } else {
          setData((java.nio.ByteBuffer)value);
        }
      }
      break;

    case NUM_ENTRIES:
      if (value == null) {
        unsetNumEntries();
      } else {
        setNumEntries((java.lang.Integer)value);
      }
      break;

    case HAS_MORE:
      if (value == null) {
        unsetHasMore();
      } else {
        setHasMore((java.lang.Boolean)value);
      }
      break;

    }
  }

  public java.lang.Object getFieldValue(_Fields field) {
    switch (field) {
    case DESC:
      return getDesc();

    case DATA:
      return getData();

    case NUM_ENTRIES:
      return getNumEntries();

    case HAS_MORE:
      return isHasMore();

    }
    throw new java.lang.IllegalStateException();
  }

  /** Returns true if field corresponding to fieldID is set (has been assigned a value) and false otherwise */
  public boolean isSet(_Fields field) {
    if (field == null) {
      throw new java.lang.IllegalArgumentException();
    }

    switch (field) {
    case DESC:
      return isSetDesc();
    case DATA:
      return isSetData();
    case NUM_ENTRIES:
      return isSetNumEntries();
    case HAS_MORE:
      return isSetHasMore();
    }
    throw new java.lang.IllegalStateException();
  }

  @Override
  public boolean equals(java.lang.Object that) {
    if (that == null)
      return false;
    if (that instanceof rpc_iterator_handle)
      return this.equals((rpc_iterator_handle)that);
    return false;
  }

  public boolean equals(rpc_iterator_handle that) {
    if (that == null)
      return false;
    if (this == that)
      return true;

    boolean this_present_desc = true && this.isSetDesc();
    boolean that_present_desc = true && that.isSetDesc();
    if (this_present_desc || that_present_desc) {
      if (!(this_present_desc && that_present_desc))
        return false;
      if (!this.desc.equals(that.desc))
        return false;
    }

    boolean this_present_data = true && this.isSetData();
    boolean that_present_data = true && that.isSetData();
    if (this_present_data || that_present_data) {
      if (!(this_present_data && that_present_data))
        return false;
      if (!this.data.equals(that.data))
        return false;
    }

    boolean this_present_num_entries = true;
    boolean that_present_num_entries = true;
    if (this_present_num_entries || that_present_num_entries) {
      if (!(this_present_num_entries && that_present_num_entries))
        return false;
      if (this.num_entries != that.num_entries)
        return false;
    }

    boolean this_present_has_more = true;
    boolean that_present_has_more = true;
    if (this_present_has_more || that_present_has_more) {
      if (!(this_present_has_more && that_present_has_more))
        return false;
      if (this.has_more != that.has_more)
        return false;
    }

    return true;
  }

  @Override
  public int hashCode() {
    int hashCode = 1;

    hashCode = hashCode * 8191 + ((isSetDesc()) ? 131071 : 524287);
    if (isSetDesc())
      hashCode = hashCode * 8191 + desc.hashCode();

    hashCode = hashCode * 8191 + ((isSetData()) ? 131071 : 524287);
    if (isSetData())
      hashCode = hashCode * 8191 + data.hashCode();

    hashCode = hashCode * 8191 + num_entries;

    hashCode = hashCode * 8191 + ((has_more) ? 131071 : 524287);

    return hashCode;
  }

  @Override
  public int compareTo(rpc_iterator_handle other) {
    if (!getClass().equals(other.getClass())) {
      return getClass().getName().compareTo(other.getClass().getName());
    }

    int lastComparison = 0;

    lastComparison = java.lang.Boolean.valueOf(isSetDesc()).compareTo(other.isSetDesc());
    if (lastComparison != 0) {
      return lastComparison;
    }
    if (isSetDesc()) {
      lastComparison = org.apache.thrift.TBaseHelper.compareTo(this.desc, other.desc);
      if (lastComparison != 0) {
        return lastComparison;
      }
    }
    lastComparison = java.lang.Boolean.valueOf(isSetData()).compareTo(other.isSetData());
    if (lastComparison != 0) {
      return lastComparison;
    }
    if (isSetData()) {
      lastComparison = org.apache.thrift.TBaseHelper.compareTo(this.data, other.data);
      if (lastComparison != 0) {
        return lastComparison;
      }
    }
    lastComparison = java.lang.Boolean.valueOf(isSetNumEntries()).compareTo(other.isSetNumEntries());
    if (lastComparison != 0) {
      return lastComparison;
    }
    if (isSetNumEntries()) {
      lastComparison = org.apache.thrift.TBaseHelper.compareTo(this.num_entries, other.num_entries);
      if (lastComparison != 0) {
        return lastComparison;
      }
    }
    lastComparison = java.lang.Boolean.valueOf(isSetHasMore()).compareTo(other.isSetHasMore());
    if (lastComparison != 0) {
      return lastComparison;
    }
    if (isSetHasMore()) {
      lastComparison = org.apache.thrift.TBaseHelper.compareTo(this.has_more, other.has_more);
      if (lastComparison != 0) {
        return lastComparison;
      }
    }
    return 0;
  }

  public _Fields fieldForId(int fieldId) {
    return _Fields.findByThriftId(fieldId);
  }

  public void read(org.apache.thrift.protocol.TProtocol iprot) throws org.apache.thrift.TException {
    scheme(iprot).read(iprot, this);
  }

  public void write(org.apache.thrift.protocol.TProtocol oprot) throws org.apache.thrift.TException {
    scheme(oprot).write(oprot, this);
  }

  @Override
  public java.lang.String toString() {
    java.lang.StringBuilder sb = new java.lang.StringBuilder("rpc_iterator_handle(");
    boolean first = true;

    sb.append("desc:");
    if (this.desc == null) {
      sb.append("null");
    } else {
      sb.append(this.desc);
    }
    first = false;
    if (!first) sb.append(", ");
    sb.append("data:");
    if (this.data == null) {
      sb.append("null");
    } else {
      org.apache.thrift.TBaseHelper.toString(this.data, sb);
    }
    first = false;
    if (!first) sb.append(", ");
    sb.append("num_entries:");
    sb.append(this.num_entries);
    first = false;
    if (!first) sb.append(", ");
    sb.append("has_more:");
    sb.append(this.has_more);
    first = false;
    sb.append(")");
    return sb.toString();
  }

  public void validate() throws org.apache.thrift.TException {
    // check for required fields
    if (desc == null) {
      throw new org.apache.thrift.protocol.TProtocolException("Required field 'desc' was not present! Struct: " + toString());
    }
    if (data == null) {
      throw new org.apache.thrift.protocol.TProtocolException("Required field 'data' was not present! Struct: " + toString());
    }
    // alas, we cannot check 'num_entries' because it's a primitive and you chose the non-beans generator.
    // alas, we cannot check 'has_more' because it's a primitive and you chose the non-beans generator.
    // check for sub-struct validity
    if (desc != null) {
      desc.validate();
    }
  }

  private void writeObject(java.io.ObjectOutputStream out) throws java.io.IOException {
    try {
      write(new org.apache.thrift.protocol.TCompactProtocol(new org.apache.thrift.transport.TIOStreamTransport(out)));
    } catch (org.apache.thrift.TException te) {
      throw new java.io.IOException(te);
    }
  }

  private void readObject(java.io.ObjectInputStream in) throws java.io.IOException, java.lang.ClassNotFoundException {
    try {
      // it doesn't seem like you should have to do this, but java serialization is wacky, and doesn't call the default constructor.
      __isset_bitfield = 0;
      read(new org.apache.thrift.protocol.TCompactProtocol(new org.apache.thrift.transport.TIOStreamTransport(in)));
    } catch (org.apache.thrift.TException te) {
      throw new java.io.IOException(te);
    }
  }

  private static class rpc_iterator_handleStandardSchemeFactory implements org.apache.thrift.scheme.SchemeFactory {
    public rpc_iterator_handleStandardScheme getScheme() {
      return new rpc_iterator_handleStandardScheme();
    }
  }

  private static class rpc_iterator_handleStandardScheme extends org.apache.thrift.scheme.StandardScheme<rpc_iterator_handle> {

    public void read(org.apache.thrift.protocol.TProtocol iprot, rpc_iterator_handle struct) throws org.apache.thrift.TException {
      org.apache.thrift.protocol.TField schemeField;
      iprot.readStructBegin();
      while (true)
      {
        schemeField = iprot.readFieldBegin();
        if (schemeField.type == org.apache.thrift.protocol.TType.STOP) { 
          break;
        }
        switch (schemeField.id) {
          case 1: // DESC
            if (schemeField.type == org.apache.thrift.protocol.TType.STRUCT) {
              if (struct.desc == null) {
                struct.desc = new rpc_iterator_descriptor();
              }
              struct.desc.read(iprot);
              struct.setDescIsSet(true);
            } else { 
              org.apache.thrift.protocol.TProtocolUtil.skip(iprot, schemeField.type);
            }
            break;
          case 2: // DATA
            if (schemeField.type == org.apache.thrift.protocol.TType.STRING) {
              struct.data = iprot.readBinary();
              struct.setDataIsSet(true);
            } else { 
              org.apache.thrift.protocol.TProtocolUtil.skip(iprot, schemeField.type);
            }
            break;
          case 3: // NUM_ENTRIES
            if (schemeField.type == org.apache.thrift.protocol.TType.I32) {
              struct.num_entries = iprot.readI32();
              struct.setNumEntriesIsSet(true);
            } else { 
              org.apache.thrift.protocol.TProtocolUtil.skip(iprot, schemeField.type);
            }
            break;
          case 4: // HAS_MORE
            if (schemeField.type == org.apache.thrift.protocol.TType.BOOL) {
              struct.has_more = iprot.readBool();
              struct.setHasMoreIsSet(true);
            } else { 
              org.apache.thrift.protocol.TProtocolUtil.skip(iprot, schemeField.type);
            }
            break;
          default:
            org.apache.thrift.protocol.TProtocolUtil.skip(iprot, schemeField.type);
        }
        iprot.readFieldEnd();
      }
      iprot.readStructEnd();

      // check for required fields of primitive type, which can't be checked in the validate method
      if (!struct.isSetNumEntries()) {
        throw new org.apache.thrift.protocol.TProtocolException("Required field 'num_entries' was not found in serialized data! Struct: " + toString());
      }
      if (!struct.isSetHasMore()) {
        throw new org.apache.thrift.protocol.TProtocolException("Required field 'has_more' was not found in serialized data! Struct: " + toString());
      }
      struct.validate();
    }

    public void write(org.apache.thrift.protocol.TProtocol oprot, rpc_iterator_handle struct) throws org.apache.thrift.TException {
      struct.validate();

      oprot.writeStructBegin(STRUCT_DESC);
      if (struct.desc != null) {
        oprot.writeFieldBegin(DESC_FIELD_DESC);
        struct.desc.write(oprot);
        oprot.writeFieldEnd();
      }
      if (struct.data != null) {
        oprot.writeFieldBegin(DATA_FIELD_DESC);
        oprot.writeBinary(struct.data);
        oprot.writeFieldEnd();
      }
      oprot.writeFieldBegin(NUM_ENTRIES_FIELD_DESC);
      oprot.writeI32(struct.num_entries);
      oprot.writeFieldEnd();
      oprot.writeFieldBegin(HAS_MORE_FIELD_DESC);
      oprot.writeBool(struct.has_more);
      oprot.writeFieldEnd();
      oprot.writeFieldStop();
      oprot.writeStructEnd();
    }

  }

  private static class rpc_iterator_handleTupleSchemeFactory implements org.apache.thrift.scheme.SchemeFactory {
    public rpc_iterator_handleTupleScheme getScheme() {
      return new rpc_iterator_handleTupleScheme();
    }
  }

  private static class rpc_iterator_handleTupleScheme extends org.apache.thrift.scheme.TupleScheme<rpc_iterator_handle> {

    @Override
    public void write(org.apache.thrift.protocol.TProtocol prot, rpc_iterator_handle struct) throws org.apache.thrift.TException {
      org.apache.thrift.protocol.TTupleProtocol oprot = (org.apache.thrift.protocol.TTupleProtocol) prot;
      struct.desc.write(oprot);
      oprot.writeBinary(struct.data);
      oprot.writeI32(struct.num_entries);
      oprot.writeBool(struct.has_more);
    }

    @Override
    public void read(org.apache.thrift.protocol.TProtocol prot, rpc_iterator_handle struct) throws org.apache.thrift.TException {
      org.apache.thrift.protocol.TTupleProtocol iprot = (org.apache.thrift.protocol.TTupleProtocol) prot;
      if (struct.desc == null) {
        struct.desc = new rpc_iterator_descriptor();
      }
      struct.desc.read(iprot);
      struct.setDescIsSet(true);
      struct.data = iprot.readBinary();
      struct.setDataIsSet(true);
      struct.num_entries = iprot.readI32();
      struct.setNumEntriesIsSet(true);
      struct.has_more = iprot.readBool();
      struct.setHasMoreIsSet(true);
    }
  }

  private static <S extends org.apache.thrift.scheme.IScheme> S scheme(org.apache.thrift.protocol.TProtocol proto) {
    return (org.apache.thrift.scheme.StandardScheme.class.equals(proto.getScheme()) ? STANDARD_SCHEME_FACTORY : TUPLE_SCHEME_FACTORY).getScheme();
  }
}

