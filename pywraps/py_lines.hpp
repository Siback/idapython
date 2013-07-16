#ifndef __PYWRAPS__LINES__
#define __PYWRAPS__LINES__

//<code(py_lines)>
//------------------------------------------------------------------------
static PyObject *py_get_user_defined_prefix = NULL;
static void idaapi s_py_get_user_defined_prefix(
  ea_t ea,
  int lnnum,
  int indent,
  const char *line,
  char *buf,
  size_t bufsize)
{
  PyObject *py_ret = PyObject_CallFunction(
    py_get_user_defined_prefix,
    PY_FMT64 "iis" PY_FMT64,
    ea, lnnum, indent, line, bufsize);

  // Error? Display it
  // No error? Copy the buffer
  if ( !PyW_ShowCbErr("py_get_user_defined_prefix") )
  {
    Py_ssize_t py_len;
    char *py_str;
    if ( PyString_AsStringAndSize(py_ret, &py_str, &py_len) != -1 )
    {
      memcpy(buf, py_str, qmin(bufsize, py_len));
      if ( py_len < bufsize )
        buf[py_len] = '\0';
    }
  }
  Py_XDECREF(py_ret);
}
//</code(py_lines)>

//------------------------------------------------------------------------

//<inline(py_lines)>

//------------------------------------------------------------------------
/*
#<pydoc>
def set_user_defined_prefix(width, callback):
    """
    User-defined line-prefixes are displayed just after the autogenerated
    line prefixes. In order to use them, the plugin should call the
    following function to specify its width and contents.
    @param width: the width of the user-defined prefix
    @param callback: a get_user_defined_prefix callback to get the contents of the prefix.
        Its arguments:
          ea     - linear address
          lnnum  - line number
          indent - indent of the line contents (-1 means the default instruction)
                   indent and is used for instruction itself. see explanations for printf_line()
          line   - the line to be generated. the line usually contains color tags this argument
                   can be examined to decide whether to generated the prefix
          bufsize- the maximum allowed size of the output buffer
        It returns a buffer of size < bufsize

    In order to remove the callback before unloading the plugin, specify the width = 0 or the callback = None
    """
    pass
#</pydoc>
*/
static PyObject *py_set_user_defined_prefix(size_t width, PyObject *pycb)
{
  if ( width == 0 || pycb == Py_None )
  {
    // Release old callback reference
    Py_XDECREF(py_get_user_defined_prefix);

    // ...and clear it
    py_get_user_defined_prefix = NULL;

    // Uninstall user defind prefix
    set_user_defined_prefix(0, NULL);
  }
  else if ( PyCallable_Check(pycb) )
  {
    // Release old callback reference
    Py_XDECREF(py_get_user_defined_prefix);

    // Copy new callback and hold a reference
    py_get_user_defined_prefix = pycb;
    Py_INCREF(py_get_user_defined_prefix);

    set_user_defined_prefix(width, s_py_get_user_defined_prefix);
  }
  else
  {
    Py_RETURN_FALSE;
  }
  Py_RETURN_TRUE;
}

//-------------------------------------------------------------------------
/*
#<pydoc>
def tag_remove(colstr):
    """
    Remove color escape sequences from a string
    @param colstr: the colored string with embedded tags
    @return:
        None on failure
        or a new string w/o the tags
    """
    pass
#</pydoc>
*/
PyObject *py_tag_remove(const char *instr)
{
  size_t sz = strlen(instr);
  char *buf = new char[sz + 5];
  if ( buf == NULL )
    Py_RETURN_NONE;

  ssize_t r = tag_remove(instr, buf, sz);
  PyObject *res;
  if ( r < 0 )
  {
    Py_INCREF(Py_None);
    res = Py_None;
  }
  else
  {
    res = PyString_FromString(buf);
  }
  delete [] buf;
  return res;
}

//-------------------------------------------------------------------------
PyObject *py_tag_addr(ea_t ea)
{
  char buf[100];
  tag_addr(buf, buf + sizeof(buf), ea);
  return PyString_FromString(buf);
}

//-------------------------------------------------------------------------
int py_tag_skipcode(const char *line)
{
  return tag_skipcode(line)-line;
}

//-------------------------------------------------------------------------
int py_tag_skipcodes(const char *line)
{
  return tag_skipcodes(line)-line;
}

//-------------------------------------------------------------------------
int py_tag_advance(const char *line, int cnt)
{
  return tag_advance(line, cnt)-line;
}

//-------------------------------------------------------------------------
/*
#<pydoc>
def generate_disassembly(ea, max_lines, as_stack, notags):
    """
    Generate disassembly lines (many lines) and put them into a buffer

    @param ea: address to generate disassembly for
    @param max_lines: how many lines max to generate
    @param as_stack: Display undefined items as 2/4/8 bytes
    @return:
        - None on failure
        - tuple(most_important_line_number, tuple(lines)) : Returns a tuple containing
          the most important line number and a tuple of generated lines
    """
    pass
#</pydoc>
*/
PyObject *py_generate_disassembly(
  ea_t ea,
  int max_lines,
  bool as_stack,
  bool notags)
{
  if ( max_lines <= 0 )
    Py_RETURN_NONE;

  qstring qbuf;
  char **lines = new char *[max_lines];
  int lnnum;
  int nlines = generate_disassembly(ea, lines, max_lines, &lnnum, as_stack);

  PyObject *py_tuple = PyTuple_New(nlines);
  for ( int i=0; i<nlines; i++ )
  {
    const char *s = lines[i];
    size_t line_len = strlen(s);
    if ( notags )
    {
      qbuf.resize(line_len+5);
      tag_remove(s, &qbuf[0], line_len);
      s = (const char *)&qbuf[0];
    }
    PyTuple_SetItem(py_tuple, i, PyString_FromString(s));
    qfree(lines[i]);
  }
  delete [] lines;
  PyObject *py_result = Py_BuildValue("(iO)", lnnum, py_tuple);
  Py_DECREF(py_tuple);
  return py_result;
}
//</inline(py_lines)>
#endif