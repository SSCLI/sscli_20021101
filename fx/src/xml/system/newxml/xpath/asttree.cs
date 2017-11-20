//------------------------------------------------------------------------------
// <copyright file="asttree.cs" company="Microsoft">
//     
//      Copyright (c) 2002 Microsoft Corporation.  All rights reserved.
//     
//      The use and distribution terms for this software are contained in the file
//      named license.txt, which can be found in the root of this distribution.
//      By using this software in any fashion, you are agreeing to be bound by the
//      terms of this license.
//     
//      You must not remove this notice, or any other, from this software.
//     
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath {
    using System;
    using System.IO;
    using System.Collections;
    using System.Xml;
    using System.Xml.Schema;

    /*--------------------------------------------------------------------------------------------- *
     * Dynamic Part Below...                                                                        *
     * -------------------------------------------------------------------------------------------- */

    // stack element class
    // this one needn't change, even the parameter in methods
    internal class AxisElement {
        internal DoubleLinkAxis curNode;                // current under-checking node during navigating
        internal int rootDepth;                         // root depth -- contextDepth + 1 if ! isDss; context + {1...} if isDss
        internal int curDepth;                          // current depth
        internal bool isMatch;                          // current is already matching or waiting for matching

        internal DoubleLinkAxis CurNode {
            get { return this.curNode; }
        }
            
        internal int RootDepth {
            get { return this.rootDepth; }
        }
        internal int CurDepth {
            get { return this.curDepth; }
        }

        internal string CurName {
            get { return this.curNode.Name; }
        }

        internal string CurPrefix {
            get { return this.curNode.Prefix; }
        }

        internal string CurURN {
            get { return this.curNode.URN; }
        }

        // constructor
        internal AxisElement (DoubleLinkAxis node, int depth) {
            this.curNode = node;
            this.rootDepth = this.curDepth = depth;
            this.isMatch = false;
        }

        internal void SetDepth (int depth) {
            this.rootDepth = this.curDepth = depth;
            return;
        }

        // "a/b/c"     pointer from b move to a
        // needn't change even tree structure changes
        internal void MoveToParent(int depth, ForwardAxis parent) {
            // "a/b/c", trying to match b (current node), but meet the end of a, so move pointer to a
            if ( depth == this.curDepth - 1 ) {
                // really need to move the current node pointer to parent
                // what i did here is for seperating the case of IsDss or only IsChild
                // bcoz in the first case i need to expect "a" from random depth
                // -1 means it doesn't expect some specific depth (referecing the dealing to -1 in movetochild method
                // while in the second case i can't change the root depth which is 1.
                if ((this.curNode.Input == parent.RootNode ) && (parent.IsDss)) {
                    this.curNode = parent.RootNode;
                    this.rootDepth = this.curDepth = -1;
                    return;
                }
                else if (this.curNode.Input != null) {      // else cur-depth --, cur-node change
                    this.curNode = (DoubleLinkAxis) (this.curNode.Input);
                    this.curDepth --;
                    return;
                }
                else return;
            }
            // "a/b/c", trying to match b (current node), but meet the end of x (another child of a)
            // or maybe i matched, now move out the current node
            // or move out after failing to match attribute
            // the node i m next expecting is still the current node
            else if (depth == this.curDepth) {              // after matched or [2] failed in matching attribute
                if (this.isMatch) {
                    this.isMatch = false;   
                }
            }
            return;                                         // this node is still what i am expecting
            // ignore
        }

        // equal & ! attribute then move
        // "a/b/c"     pointer from a move to b
        // return true if reach c and c is an element and c is the axis
        internal bool MoveToChild(string name, string URN, int depth, ForwardAxis parent) { 
            // an attribute can never be the same as an element
            if (Asttree.IsAttribute(this.curNode)) {
                return false; 
            }

            // either moveToParent or moveToChild status will have to be changed into unmatch...
            if (this.isMatch) {
                this.isMatch = false;    
            }
            if (! AxisStack.Equal (this.curNode.Name, this.curNode.URN, name, URN)) {
                return false; 
            }
            if (this.curDepth == -1) {
                SetDepth (depth); 
            }
            else if (depth > this.curDepth) {
                return false; 
            }
            // matched ...  
            if (this.curNode == parent.TopNode) {
                this.isMatch = true;
                return true;                
            }
            // move down this.curNode
            DoubleLinkAxis nowNode = (DoubleLinkAxis) (this.curNode.Next);
            if (Asttree.IsAttribute (nowNode)) {
                this.isMatch = true;                    // for attribute 
                return false;
            }
            this.curNode = nowNode;
            this.curDepth ++;
            return false;
        }

    }

    internal class AxisStack {
        // property
        private ArrayList stack;                            // of AxisElement
        private ForwardAxis subtree;                        // reference to the corresponding subtree
        private ActiveAxis parent;

        internal ForwardAxis Subtree {
            get { return this.subtree; }
        }

        internal int Length {                               // stack length
            get {return this.stack.Count; }
        }
    
        // instructor
        public AxisStack (ForwardAxis faxis, ActiveAxis parent) {
            this.subtree = faxis;
            this.stack = new ArrayList();
            this.parent = parent;       // need to use its contextdepth each time....

            // improvement:
            // if ! isDss, there has nothing to do with Push/Pop, only one copy each time will be kept
            // if isDss, push and pop each time....
            if (! faxis.IsDss) {                // keep an instance
                this.Push (1);              // context depth + 1
            }
            // else just keep stack empty
        }

        // method
        internal void Push (int depth) {
            AxisElement eaxis = new AxisElement (this.subtree.RootNode, depth);
            this.stack.Add (eaxis);
        }

        internal void Pop () {
            this.stack.RemoveAt (Length - 1);
        }

        // used in the beginning of .//  and MoveToChild
        // didn't consider Self, only consider name
        internal static bool Equal (string thisname, string thisURN, string name, string URN) {
            // which means "b" in xpath, no namespace should be specified
            if (thisURN == null) {
                if ( !((URN == null) || (URN == string.Empty))) {
                    return false;
                }
            }
            // != "*"
            else if ((thisURN != string.Empty) && (thisURN != URN)) {
                return false; 
            }
            // != "a:*" || "*"
            if ((thisname != string.Empty) && (thisname != name)) {
                return false; 
            }
            return true;
        }

        // "a/b/c"     pointer from b move to a
        // needn't change even tree structure changes
        internal void MoveToParent(string name, string URN, int depth) {
            if (this.subtree.IsSelfAxis) {
                return; 
            }

            foreach (AxisElement eaxis in this.stack) {
                eaxis.MoveToParent (depth, this.subtree);
            }

            // in ".//"'s case, since each time you push one new element while match, why not pop one too while match?
            if ( this.subtree.IsDss && Equal (this.subtree.RootNode.Name, this.subtree.RootNode.URN, name, URN)) {
                Pop();  
            }   // only the last one
        }

        // "a/b/c"     pointer from a move to b
        // return true if reach c
        internal bool MoveToChild(string name, string URN, int depth) {
            bool result = false;
            // push first
            if ( this.subtree.IsDss && Equal (this.subtree.RootNode.Name, this.subtree.RootNode.URN, name, URN)) {      
                Push(-1); 
            }   
            foreach (AxisElement eaxis in this.stack) {
                if (eaxis.MoveToChild (name, URN, depth, this.subtree)) {
                    result = true;
                }
            }
            return result;
        }

        // attribute can only at the topaxis part
        // dealing with attribute only here, didn't go into stack element at all
        // stack element only deal with moving the pointer around elements
        internal bool MoveToAttribute(string name, string URN, int depth) {
            if (! this.subtree.IsAttribute) {
                return false;
            }
            if (! Equal (this.subtree.TopNode.Name, this.subtree.TopNode.URN, name, URN) ) {
                return false;
            }

            bool result = false;

            // no stack element for single attribute, so dealing with it seperately
            if (this.subtree.TopNode.Input == null) {
                return (this.subtree.IsDss || (depth == 1));
            }

            foreach (AxisElement eaxis in this.stack) {
                if ((eaxis.isMatch) && ( eaxis.CurNode == this.subtree.TopNode.Input )) {
                    result = true;
                }
            }
            return result;
        }

    }

    //******************************************************************************
    //******************************************************************************
    // whenever an element is under identity-constraint, an instance of this class will be called
    // only care about property at this time
    internal class ActiveAxis : ForwardOnlyXpathExpr {
        // consider about reactivating....  the stack should be clear right??
        // just reset contextDepth & isActive....
        private int currentDepth;                       // current depth, trace the depth by myself... movetochild, movetoparent, movetoattribute
        private bool isActive;                          // not active any more after moving out context node
        private Asttree axisTree;                       // reference to the whole tree
        // for each subtree i need to keep a stack...
        private ArrayList axisStack;                    // of AxisStack

        internal bool IsActive {
            get { return this.isActive; }
        }

        public int CurrentDepth {
            get { return this.currentDepth; }
        }

        // if an instance is !IsActive, then it can be reactive and reuse
        // still need thinking.....
        internal void Reactivate () {
            this.isActive = true; 
            this.currentDepth = -1;
        }

        internal ActiveAxis (Asttree axisTree) {
            this.axisTree = axisTree;                                               // only a pointer.  do i need it?
            this.currentDepth = -1;                                                 // context depth is 0 -- enforce moveToChild for the context node
                                                                                    // otherwise can't deal with "." node
            this.axisStack = new ArrayList(axisTree.SubtreeArray.Count);            // defined length
            // new one stack element for each one
            foreach (ForwardAxis faxis in axisTree.SubtreeArray) {
                AxisStack stack = new AxisStack (faxis, this);
                axisStack.Add (stack);
            }
            this.isActive = true;
        }

        public bool MoveToStartElement (string localname, string URN) {
            if (!isActive) {
                return false;
            }

            // for each:
            this.currentDepth ++;
            bool result = false;
            foreach (AxisStack stack in this.axisStack) {
                // special case for self tree   "." | ".//."
                if (stack.Subtree.IsSelfAxis) {
                    if ( stack.Subtree.IsDss || (this.CurrentDepth == 0))
                        result = true;
                    continue;
                }

                // otherwise if it's context node then return false
                if (this.CurrentDepth == 0) continue;

                if (stack.MoveToChild (localname, URN, this.currentDepth)) {
                    result = true;
                    // even already know the last result is true, still need to continue...
                    // run everyone once
                }
            }
            return result;
        }
            
        public void EndElement (string localname, string URN) {
            // need to think if the early quitting will affect reactivating....
            if (this.currentDepth ==  0) {          // leave context node
                this.isActive = false;
            }
            if (! this.isActive) {
                return;
            }
            foreach (AxisStack stack in this.axisStack) {
                stack.MoveToParent (localname, URN, this.currentDepth);
            }
            this.currentDepth -- ;
            return;
        }

        //******************************* Secondly field interface ***************************
        public bool MoveToAttribute (string localname, string URN) {
            if (! this.isActive) {
                return false;
            }
            bool result = false;
            foreach (AxisStack stack in this.axisStack) {
                if (stack.MoveToAttribute (localname, URN, this.currentDepth + 1)) {  // don't change depth for attribute, but depth is add 1 
                    result = true;
                }
            }
            return result;
        }
    }

    /* ---------------------------------------------------------------------------------------------- *
     * Static Part Below...                                                                           *
     * ---------------------------------------------------------------------------------------------- */

    // each node in the xpath tree
    internal class DoubleLinkAxis : Axis {
        internal Axis next;

        internal Axis Next {
            get { return this.next; }
            set { this.next = value; }
        }

        //constructor
        // I can't recrusively call constructor -- "No overload for method 'Axis' takes '0' arguments"
        // or assign value to 'this' -- "Cannot assign to '<this>' because it is read-only"

        //constructor
        internal DoubleLinkAxis ( Axis input, 
            Axis next, 
            String urn, 
            String prefix, 
            String name, 
            XPathNodeType nodetype, 
            AxisType axistype,
            bool abbrAxis ) {
            this._input = input;
            this.next = next;
            this._urn = urn;
            this._prefix = prefix;
            this._name = name;
            this._nodetype = nodetype;
            this._axistype = axistype;
            this.abbrAxis = abbrAxis;
        }

        //constructor
        internal DoubleLinkAxis (Axis axis, DoubleLinkAxis inputaxis) {
            this.next = null;
            this._input = inputaxis;
            this._urn = axis.URN;
            this._prefix = axis.Prefix;
            this._name = axis.Name;
            this._nodetype = axis.Type;
            this._axistype = axis.TypeOfAxis;
            this.abbrAxis = axis.abbrAxis;
            if (inputaxis != null) {
                inputaxis.Next = this;
            }
        }

        // recursive here
        internal static DoubleLinkAxis ConvertTree (Axis axis) {
            if (axis == null) {
                return null;
            }
            return ( new DoubleLinkAxis (axis, ConvertTree ((Axis) (axis.Input))));
        }
    }



    // only keep axis, rootNode, isAttribute, isDss inside
    // act as an element tree for the Asttree
    internal class ForwardAxis {
        // Axis tree
        private DoubleLinkAxis topNode;
        private DoubleLinkAxis rootNode;                // the root for reverse Axis

        // Axis tree property
        private bool isAttribute;                       // element or attribute?      "@"?
        private bool isDss;                             // has ".//" in front of it?
        private bool isSelfAxis;                        // only one node in the tree, and it's "." (self) node

        internal DoubleLinkAxis RootNode { 
            get { return this.rootNode; }
        }

        internal DoubleLinkAxis TopNode {
            get { return this.topNode; }
        }

        internal bool IsAttribute {
            get { return this.isAttribute; }
        }

        // has ".//" in front of it?
        internal bool IsDss {
            get { return this.isDss; }
        }

        internal bool IsSelfAxis {
            get { return this.isSelfAxis; }
        }

        public ForwardAxis (DoubleLinkAxis axis, bool isdesorself) {
            this.isDss = isdesorself;
            this.isAttribute = Asttree.IsAttribute (axis);
            this.topNode = axis;
            this.rootNode = axis;
            while ( this.rootNode.Input != null ) {
                this.rootNode = (DoubleLinkAxis)(this.rootNode.Input);
            }
            // better to calculate it out, since it's used so often, and if the top is self then the whole tree is self
            this.isSelfAxis = Asttree.IsSelf (this.topNode);
        }
    }

    // static, including an array of ForwardAxis  (this is the whole picture)
    internal class Asttree {
        // set private then give out only get access, to keep it intact all along
        private ArrayList fAxisArray;           
        private string xpathexpr;
        private bool isField;                                   // field or selector
        private XmlNamespaceManager nsmgr;

        internal ArrayList SubtreeArray {
            get { return fAxisArray; }
        }

        // when making a new instance for Asttree, we do the compiling, and create the static tree instance
        public Asttree (string xPath, bool isField, XmlNamespaceManager nsmgr) {
            this.xpathexpr = xPath;
            this.isField = isField;
            this.nsmgr = nsmgr;
            // checking grammar... and build fAxisArray
            this.CompileXPath (xPath, isField, nsmgr);          // might throw exception in the middle
        }

        // only for debug
#if DEBUG
        public void PrintTree (StreamWriter msw) {
            foreach (ForwardAxis axis in fAxisArray) {
                msw.WriteLine ("<Tree IsDss=\"{0}\" IsAttribute=\"{1}\">" , axis.IsDss, axis.IsAttribute);
                DoubleLinkAxis printaxis = axis.TopNode;
                while ( printaxis != null ) {
                    msw.WriteLine (" <node>");
                    msw.WriteLine ("  <URN> {0} </URN>", printaxis.URN);
                    msw.WriteLine ("  <Prefix> {0} </Prefix>", printaxis.Prefix);
                    msw.WriteLine ("  <Name> {0} </Name>", printaxis.Name);
                    msw.WriteLine ("  <NodeType> {0} </NodeType>", printaxis.Type);
                    msw.WriteLine ("  <AxisType> {0} </AxisType>", printaxis.TypeOfAxis);
                    msw.WriteLine (" </node>");
                    printaxis = (DoubleLinkAxis) (printaxis.Input);
                }
                msw.WriteLine ("</Tree>");
            }
        }
#endif

        // this part is for parsing restricted xpath from grammar
        private static bool IsNameTest(Axis ast) {
            // Type = Element, abbrAxis = false
            // all are the same, has child:: or not
            return ((ast.TypeOfAxis == Axis.AxisType.Child) && (ast.Type == XPathNodeType.Element));
        }

        internal static bool IsAttribute(Axis ast) {
            return ((ast.TypeOfAxis == Axis.AxisType.Attribute) && (ast.Type == XPathNodeType.Attribute));
        }

        private static bool IsDescendantOrSelf(Axis ast) {
            return ((ast.TypeOfAxis == Axis.AxisType.DescendantOrSelf) && (ast.Type == XPathNodeType.All) && (ast.abbrAxis));
        }

        internal static bool IsSelf(Axis ast) {
            return ((ast.TypeOfAxis == Axis.AxisType.Self) && (ast.Type == XPathNodeType.All) && (ast.abbrAxis));
        }

        // don't return true or false, if it's invalid path, just throw exception during the process
        // for whitespace thing, i will directly trim the tree built here...
        public void CompileXPath (string xPath, bool isField, XmlNamespaceManager nsmgr) {
            if ((xPath == null) || (xPath == string.Empty)) {
                throw new XmlSchemaException(Res.Sch_EmptyXPath);
            }

            // firstly i still need to have an ArrayList to store tree only...
            // can't new ForwardAxis right away
            string[] xpath = xPath.Split('|');
            ArrayList AstArray = new ArrayList(xpath.Length);
            this.fAxisArray = new ArrayList(xpath.Length);

            // throw compile exceptions
            // can i only new one builder here then run compile several times??
            try {
                foreach (string value in xpath) {
                    // default ! isdesorself (no .//)
                    Axis ast = (Axis) (XPathParser.ParseXPathExpresion(value));
                    AstArray.Add (ast);
                }
            }   
            catch {
                throw  new XmlSchemaException(Res.Sch_ICXpathError, xPath);
            }

            Axis stepAst;
            foreach (Axis ast in AstArray) {
                // Restricted form
                // field can have an attribute:

                // throw exceptions during casting
                if ((stepAst = ast) == null) {
                    throw new XmlSchemaException(Res.Sch_ICXpathError, xPath);
                }

                Axis top = stepAst;

                // attribute will have namespace too
                // field can have top attribute
                if (IsAttribute (stepAst)) {
                    if (! isField) {
                        throw new XmlSchemaException(Res.Sch_SelectorAttr, xPath);
                    }
                    else {
                        SetURN (stepAst, nsmgr);
                        try {
                            stepAst = (Axis) (stepAst.Input);
                        }
                        catch {
                            throw new XmlSchemaException(Res.Sch_ICXpathError, xPath);
                        }
                    }
                }

                // field or selector
                while ((stepAst != null) && (IsNameTest (stepAst) || IsSelf (stepAst))) {
                    // trim tree "." node, if it's not the top one
                    if (IsSelf (stepAst) && (ast != stepAst)) {
                        top.Input = stepAst.Input;
                    }
                    else {
                        top = stepAst;
                        // set the URN
                        if (IsNameTest(stepAst)) {
                            SetURN (stepAst, nsmgr);
                        }
                    }
                    try {
                        stepAst = (Axis) (stepAst.Input);
                    }
                    catch {
                        throw new XmlSchemaException(Res.Sch_ICXpathError, xPath);
                    }
                }

                // the rest part can only be .// or null
                // trim the rest part, but need compile the rest part first
                top.Input = null;
                if (stepAst == null) {      // top "." and has other element beneath, trim this "." node too
                    if (IsSelf(ast) && (ast.Input != null)) {
                        this.fAxisArray.Add ( new ForwardAxis ( DoubleLinkAxis.ConvertTree ((Axis) (ast.Input)), false));
                    }
                    else {
                        this.fAxisArray.Add ( new ForwardAxis ( DoubleLinkAxis.ConvertTree (ast), false));
                    }
                    continue;
                }
                if (! IsDescendantOrSelf (stepAst)) {
                    throw new XmlSchemaException(Res.Sch_ICXpathError, xPath);
                }
                try {
                    stepAst = (Axis) (stepAst.Input);
                }
                catch {
                    throw new XmlSchemaException(Res.Sch_ICXpathError, xPath);
                }
                if ((stepAst == null) || (! IsSelf (stepAst)) || (stepAst.Input != null)) {
                    throw new XmlSchemaException(Res.Sch_ICXpathError, xPath);
                }

                // trim top "." if it's not the only node
                if (IsSelf(ast) && (ast.Input != null)) {
                    this.fAxisArray.Add ( new ForwardAxis ( DoubleLinkAxis.ConvertTree ((Axis) (ast.Input)), true));
                }
                else {
                    this.fAxisArray.Add ( new ForwardAxis ( DoubleLinkAxis.ConvertTree (ast), true));
                }
            }
        }

        // depending on axis.Name & axis.Prefix, i will set the axis._urn;
        // also, record urn from prefix during this
        // 4 different types of element or attribute (with @ before it) combinations: 
        // (1) a:b (2) b (3) * (4) a:*
        // i will check xpath to be strictly conformed from these forms
        // for (1) & (4) i will have URN set properly
        // for (2) the URN is null
        // for (3) the URN is empty
        private void SetURN (Axis axis, XmlNamespaceManager nsmgr) {
            if (axis.Prefix != String.Empty) {      // (1) (4)
                XmlNameTable nameTable = nsmgr.NameTable;
                if (nameTable != null) {
                    axis._urn = nsmgr.LookupNamespace(nsmgr.NameTable.Get(axis.Prefix));        
                }
                else {
                    axis._urn = nsmgr.LookupNamespace(axis.Prefix);     
                }
                // axis._urn = prefix == null ? null : nsmgr.LookupNamespace(prefix);       
                // wrong prefix, throw exceptions
                if (axis._urn == null) {
                    throw new XmlSchemaException(Res.Sch_UnresolvedPrefix, axis.Prefix);
                }
            }
            else if (axis.Name != String.Empty) {   // (2)
                axis._urn = null;
            }
            else {                                  // (3)
                axis._urn = "";
            }
        }

    }

}
