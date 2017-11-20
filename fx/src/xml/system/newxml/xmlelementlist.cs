//------------------------------------------------------------------------------
// <copyright file="XmlElementList.cs" company="Microsoft">
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

namespace System.Xml 
{
    using System;
    using System.Collections;
    using System.Diagnostics;

    internal class XmlElementList: XmlNodeList {
        string      asterisk;
        int         changeCount; //recording the total number that the dom tree has been changed ( insertion and deletetion )
        //the member vars below are saved for further reconstruction        
        string      name;         //only one of 2 string groups will be initialized depends on which constructor is called.
        string      localName; 
        string      namespaceURI;
        XmlNode     rootNode;
        // the memeber vars belwo serves the optimization of accessing of the elements in the list
        int         curInd;       // -1 means the starting point for a new search round
        XmlNode     curElem;      // if sets to rootNode, means the starting point for a new search round
        bool        empty;        // whether the list is empty
        bool        atomized;     //whether the localname and namespaceuri are aomized

        private XmlElementList( XmlNode parent) {
            Debug.Assert ( parent != null );
            Debug.Assert( parent.NodeType == XmlNodeType.Element || parent.NodeType == XmlNodeType.Document );
            this.rootNode = parent;
            Debug.Assert( parent.Document != null );
            parent.Document.NodeInserted +=  new XmlNodeChangedEventHandler( this.OnListChanged ) ;
            parent.Document.NodeRemoved +=  new XmlNodeChangedEventHandler( this.OnListChanged ) ;
            this.curInd = -1;
            this.curElem = rootNode;
            this.changeCount = 0;
            this.empty = false;
            this.atomized = true; 
        }

        private void OnListChanged( object sender, XmlNodeChangedEventArgs args ) {
            if( atomized == false ) {
                this.localName = this.rootNode.Document.NameTable.Add( this.localName );
                this.namespaceURI = this.rootNode.Document.NameTable.Add( this.namespaceURI );
                this.atomized = true;
            }                
            if ( IsMatch( args.Node ) ) {
                this.changeCount++ ;
                this.curInd = -1;
                this.curElem = rootNode;
                if( args.Action == XmlNodeChangedAction.Insert )
                    this.empty = false;
            }
        }

        internal XmlElementList( XmlNode parent, string name ): this( parent ) {
            Debug.Assert( parent.Document != null );            
            XmlNameTable nt = parent.Document.NameTable;
            Debug.Assert( nt != null );
            asterisk = nt.Add("*");
            this.name = nt.Add( name );
            this.localName = null;
            this.namespaceURI = null;
        }

        internal XmlElementList( XmlNode parent, string localName, string namespaceURI ): this( parent ) {
            Debug.Assert( parent.Document != null );
            XmlNameTable nt = parent.Document.NameTable;
            Debug.Assert( nt != null );
            asterisk = nt.Add("*");
            this.localName = nt.Get( localName );
            this.namespaceURI = nt.Get( namespaceURI );
            if( (this.localName == null) || (this.namespaceURI== null) ) {
                this.empty = true;
                this.atomized = false;
                this.localName = localName;
                this.namespaceURI = namespaceURI;
            }   
                this.name = null;
        }
        
        internal int ChangeCount {
            get { return changeCount; }
        }

        // return the next element node that is in PreOrder
        private XmlNode NextElemInPreOrder( XmlNode curNode ) {
            Debug.Assert( curNode != null );
            //For preorder walking, first try its child
            XmlNode retNode = curNode.FirstChild;
            if ( retNode == null ) {
                //if no child, the next node forward will the be the NextSibling of the first ancestor which has NextSibling
                //so, first while-loop find out such an ancestor (until no more ancestor or the ancestor is the rootNode
                retNode = curNode;
                while ( retNode != null 
                        && retNode != rootNode 
                        && retNode.NextSibling == null ) {
                    retNode = retNode.ParentNode;
                }
                //then if such ancestor exists, set the retNode to its NextSibling
                if ( retNode != null && retNode != rootNode )
                    retNode = retNode.NextSibling;
            }
            if ( retNode == this.rootNode ) 
                //if reach the rootNode, consider having walked through the whole tree and no more element after the curNode
                retNode = null;
            return retNode;
        }

        // return the previous element node that is in PreOrder
        private XmlNode PrevElemInPreOrder( XmlNode curNode ) {
            Debug.Assert( curNode != null );
            //For preorder walking, the previous node will be the right-most node in the tree of PreviousSibling of the curNode
            XmlNode retNode = curNode.PreviousSibling;
            // so if the PreviousSibling is not null, going through the tree down to find the right-most node
            while ( retNode != null ) {
                if ( retNode.LastChild == null )
                    break;
                retNode = retNode.LastChild;
            }
            // if no PreviousSibling, the previous node will be the curNode's parentNode
            if ( retNode == null )
                retNode = curNode.ParentNode;
            // if the final retNode is rootNode, consider having walked through the tree and no more previous node
            if ( retNode == this.rootNode )
                retNode = null;
            return retNode;
        }

        // if the current node a matching element node
        private bool IsMatch ( XmlNode curNode ) {
            if (curNode.NodeType == XmlNodeType.Element) {
                if ( this.name != null ) {
                    if ( Ref.Equal(this.name, asterisk) || Ref.Equal(curNode.Name, this.name) )
                        return true;
                } 
                else {
                    if (
                        (Ref.Equal(this.localName, asterisk) || Ref.Equal(curNode.LocalName, this.localName) ) && 
                        (Ref.Equal(this.namespaceURI, asterisk) || curNode.NamespaceURI == this.namespaceURI )
                    ) {
                        return true;
                    }
                }
            }     
            return false;
        }

        private XmlNode GetMatchingNode( XmlNode n, bool bNext ) {
            Debug.Assert( n!= null );
            XmlNode node = n;            
            do {
                if ( bNext )
                    node = NextElemInPreOrder( node );
                else
                    node = PrevElemInPreOrder( node );
            } while ( node != null && !IsMatch( node ) );
            return node;
        }

        private XmlNode GetNthMatchingNode( XmlNode n, bool bNext, int nCount ) {
            Debug.Assert( n!= null );
            XmlNode node = n;
            for ( int ind = 0 ; ind < nCount; ind++ ) {
                node = GetMatchingNode( node, bNext );
                if ( node == null )
                    return null;
            } 
            return node;
        }

        //the function is for the enumerator to find out the next available matching element node
        public XmlNode GetNextNode( XmlNode n ) {
            if( this.empty == true )
                return null;
            XmlNode node = ( n == null ) ? rootNode : n;
            return GetMatchingNode( node, true );
        }

        public override XmlNode Item(int index) {
            if ( rootNode == null || index < 0 ) 
                return null;

            if( this.empty == true )
                return null;
            if ( curInd == index )
                return curElem;
            int nDiff = index - curInd;
            bool bForward = ( nDiff > 0 );
            if ( nDiff < 0 )
                nDiff = -nDiff;
            XmlNode node;
            if ( ( node = GetNthMatchingNode( curElem, bForward, nDiff ) ) != null ) {
                curInd = index;
                curElem = node;
                return curElem;
            } 
            return null;
        }

        public override int Count { 
            get { 
                if( this.empty == true )
                    return 0;
                int nCount = 0;
                XmlNode node = rootNode;
                while ( ( node = GetMatchingNode( node, true ) ) != null ) 
                    nCount++;
                return nCount;
            }
        }
    
        public override IEnumerator GetEnumerator() {
            if( this.empty == true )
                return new XmlEmptyElementListEnumerator(this);;
            return new XmlElementListEnumerator(this);
        }
    }

     internal class XmlElementListEnumerator : IEnumerator {
        XmlElementList  list;
        XmlNode         curElem;
        int             changeCount; //save the total number that the dom tree has been changed ( insertion and deletetion ) when this enumerator is created

        public XmlElementListEnumerator( XmlElementList list ) {
            this.list = list;
            this.curElem = null;
            this.changeCount = list.ChangeCount;
        }

        public bool MoveNext() { 
            if ( list.ChangeCount != this.changeCount ) {
                //the number mismatch, there is new change(s) happened since last MoveNext() is called.
                throw new InvalidOperationException( Res.GetString(Res.Xdom_Enum_ElementList) );
            } 
            else  {
                curElem = list.GetNextNode( curElem );                
            }
            return curElem != null;
        }

        public void Reset() {
            curElem = null;
            //reset the number of changes to be synced with current dom tree as well
            this.changeCount = list.ChangeCount;
        }

        public object Current {
            get { return curElem; }
        }
    }
    
    internal class XmlEmptyElementListEnumerator : IEnumerator {        
        public XmlEmptyElementListEnumerator( XmlElementList list ) {
        }

        public bool MoveNext() { 
            return false;
        }

        public void Reset() {
        }
        
        public object Current {
            get { return null; }
        }
    }
}
