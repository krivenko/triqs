# This module defines the function parse that
# call libclang to parse a C++ file, and retrieve
# from the clang AST the classes, functions, methods, members (including
# template).
# This module is use e..g by the wrapper desc generator.
import sys,re,os
import clang.cindex
import itertools
from mako.template import Template
import textwrap

def get_annotations(node):
    return [c.displayname for c in node.get_children()
            if c.kind == clang.cindex.CursorKind.ANNOTATE_ATTR]

def process_doc (doc) :
    if not doc : return ""
    for p in ["/\*","\*/","^\s*\*", "///", "//", r"\\brief"] : doc = re.sub(p,"",doc,flags = re.MULTILINE)
    return doc.strip()

file_locations = set(())

def decay(s) :
    for tok in ['const', '&&', '&'] :
        s = re.sub(tok,'',s)
    return s.strip()

class member_(object):
    def __init__(self, cursor,ns=()):
        loc = cursor.location.file.name
        if loc : file_locations.add(loc)
        self.doc = process_doc(cursor.raw_comment)
        self.ns = ns
        self.name = cursor.spelling
        self.access = cursor.access_specifier
        self.type = type_(cursor.type)
        self.ctype = cursor.type.spelling
        self.annotations = get_annotations(cursor)
        # the declaration split in small tokens
        tokens = [t.spelling for t in cursor.get_tokens()]
        self.initializer = None
        if '=' in tokens:
            self.initializer = ''.join(tokens[tokens.index('=') + 1:tokens.index(';')])

    def namespace(self) : 
        return "::".join(self.ns)

class type_(object):
    def __init__(self, cursor):
        self.name, self.canonical_name = cursor.spelling, cursor.get_canonical().spelling

    def __repr__(self) :
        return "type : %s"%(self.name)
        #return "type : %s %s\n"%(self.name, self.canonical_name)

class Function(object):
    def __init__(self, cursor, is_constructor = False, ns=() ): #, template_list  =()):
        loc = cursor.location.file.name
        if loc : file_locations.add(loc)
        self.doc = process_doc(cursor.raw_comment)
        self.brief_doc = self.doc.split('\n')[0].strip() # improve ...
        self.ns = ns
        self.name = cursor.spelling
        self.annotations = get_annotations(cursor)
        self.access = cursor.access_specifier
        self.params = [] # a list of tuple (type, name, default_value or None).
        self.template_list = []  #template_list
        self.is_constructor = is_constructor
        self.is_static = cursor.is_static_method()
        self.parameter_arg = None # If exists, it is the parameter class
        for c in cursor.get_children():
            if c.kind == clang.cindex.CursorKind.TEMPLATE_TYPE_PARAMETER : 
                 self.template_list.append(c.spelling)
            elif (c.kind == clang.cindex.CursorKind.PARM_DECL) :
                default_value = None
                for ch in c.get_children() :
                    # TODO : string literal do not work.. needs to use location ? useful ?
                    if ch.kind in [clang.cindex.CursorKind.INTEGER_LITERAL, clang.cindex.CursorKind.FLOATING_LITERAL, 
                                   clang.cindex.CursorKind.CHARACTER_LITERAL, clang.cindex.CursorKind.STRING_LITERAL, 
                                   clang.cindex.CursorKind.UNARY_OPERATOR, clang.cindex.CursorKind.UNEXPOSED_EXPR, 
                                   clang.cindex.CursorKind.CXX_BOOL_LITERAL_EXPR ] :
                        #print [x.spelling for x in ch.get_tokens()]
                        #default_value =  ch.get_tokens().next().spelling 
                        default_value =  ''.join([x.spelling for x in ch.get_tokens()][:-1])
                t = type_(c.type)

                # We look if this argument is a parameter class...
                if 'use_parameter_class' in self.annotations : 
                    tt = c.type.get_declaration()  # guess it is not a ref
                    if not tt.location.file : tt = c.type.get_pointee().get_declaration() # it is a T &
                    #if tt.raw_comment and 'triqs:is_parameter' in tt.raw_comment:
                    self.parameter_arg = Class(tt, ns)

                self.params.append ( (t, c.spelling, default_value ))
              #else : 
            #    print " node in fun ", c.kind
        if self.parameter_arg : assert len(self.params) == 1, "When using a parameter class, it must have exactly one argument"
        self.rtype = type_(cursor.result_type) if not is_constructor else None

    def namespace(self) : 
        return "::".join(self.ns)

    def signature_cpp(self) :
        s = "{name} ({args})" if not self.is_constructor else "{rtype} {name} ({args})"
        s = s.format(args = ', '.join( ["%s %s"%(t.name,n) + "="%d if d else "" for t,n,d in self.params]), **self.__dict__) 
        if self.template_list : 
            s = "template<" + ', '.join(['typename ' + x for x in self.template_list]) + ">  " + s
        if self.is_static : s = "static " + s
        return s.strip()

    @property
    def is_template(self) : return len(self.template_list)>0

    def __str__(self) :
        return "%s\n%s\n"%(self.signature_cpp(),self.doc)

class Class(object):
    def __init__(self, cursor,ns):
        loc = cursor.location.file.name
        if loc : file_locations.add(loc)
        self.doc = process_doc(cursor.raw_comment)
        self.brief_doc = self.doc.split('\n')[0].strip() # improve ...
        self.ns = ns
        self.name = cursor.spelling
        self.functions = []
        self.constructors = []
        self.methods = []
        self.members = []
        self.proplist = []
        self.annotations = get_annotations(cursor)
        self.file = cursor.location.file.name

        # MISSING : constructors template not recognized 
        for c in cursor.get_children():
            # Only public nodes
            if c.access_specifier != clang.cindex.AccessSpecifier.PUBLIC : continue

            if (c.kind == clang.cindex.CursorKind.FIELD_DECL):
                m = member_(c)
                self.members.append(m)

            elif (c.kind == clang.cindex.CursorKind.CXX_METHOD):
                f = Function(c)
                self.methods.append(f)

            elif (c.kind == clang.cindex.CursorKind.CONSTRUCTOR):
                f = Function(c, is_constructor = True)
                self.constructors.append(f)

            elif (c.kind == clang.cindex.CursorKind.FUNCTION_DECL):
                f = Function(c)
                self.functions.append(f)
            
            elif (c.kind == clang.cindex.CursorKind.FUNCTION_TEMPLATE):
                f = Function(c)
                self.methods.append(f)

    def namespace(self) : 
        return "::".join(self.ns)

    def canonical_name(self) : return self.namespace() + '::' + self.name

    def __str__(self) :
        s,s2 = "class {name}:\n  {doc}\n\n".format(**self.__dict__),[]
        for m in self.members :
            s2 += ["%s %s"%(m.ctype,m.name)]
        for m in self.methods : 
           s2 += str(m).split('\n')
        for m in self.functions : 
           s2 += ("friend " + str(m)).split('\n')
        s2 = '\n'.join( [ "   " + l.strip() + '\n' for l in s2 if l.strip()])
        return s + s2

    def __repr__(self) : 
        return "Class %s"%self.name

def build_functions_and_classes(cursor, namespaces=[]):
    classes,functions = [],[]
    for c in cursor.get_children():
        if (c.kind == clang.cindex.CursorKind.FUNCTION_DECL
            and c.location.file.name == sys.argv[1]):
            functions.append( Function(c,is_constructor = False, ns =namespaces))
        elif (c.kind in [clang.cindex.CursorKind.CLASS_DECL, clang.cindex.CursorKind.STRUCT_DECL]
            and c.location.file.name == sys.argv[1]):
            classes.append( Class(c,namespaces))
        elif c.kind == clang.cindex.CursorKind.NAMESPACE:
            child_fnt, child_classes = build_functions_and_classes(c, namespaces +[c.spelling])
            functions.extend(child_fnt)
            classes.extend(child_classes)

    return functions,classes

def parse(filename, debug, compiler_options, where_is_libclang): 

  compiler_options =  [ '-std=c++11', '-stdlib=libc++'] + compiler_options

  clang.cindex.Config.set_library_file(where_is_libclang)
  index = clang.cindex.Index.create()
  print "Parsing the C++ file (may take a few seconds) ..."
  #print filename, ['-x', 'c++'] + compiler_options
  translation_unit = index.parse(filename, ['-x', 'c++'] + compiler_options)
  print "... done. \nExtracting ..."
 
  # If clang encounters errors, we report and stop
  errors = [d for d in translation_unit.diagnostics if d.severity >= 3]
  if errors : 
      s =  "Clang reports the following errors in parsing\n"
      for err in errors :
        loc = err.location
        s += '\n'.join(["file %s line %s col %s"%(loc.file, loc.line, loc.column), err.spelling])
      raise RuntimeError, s + "\n... Your code must compile before making the wrapper !" 

  # Analyze the AST to extract classes and functions
  functions, classes = build_functions_and_classes(translation_unit.cursor)
  print "... done"

  global file_locations
  #if len(file_locations) != 1 : 
  #    print file_locations
  #    raise RuntimeError, "Multiple file location not implemented"
  file_locations = list(file_locations)

  if debug : 
      print "functions"
      for f in functions : 
          print f
      
      print "classes"
      for c in classes : 
          print c

  return functions, classes

