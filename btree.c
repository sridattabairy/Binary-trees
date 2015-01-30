/**************************************************
 *                                                *
 *            B-tree operations     			  *
 *                                                *
 **************************************************/

/* Header files */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define   MAX      1024
#define   SIZE    10000

#define   ALLOC		  0
#define   ADD	      1
#define   FIND        2 
#define	  PRINT       3 
#define   END         4
#define   RESET		  5

/* Global declarations */

/* Btree node */
typedef struct {  
	int  n;      	/* Number of keys in node */ 
	int  *key;   	/* Node's keys */ 
	long *child; 	/* Node's child subtree offsets */ 
	long parent;	/* Node's parent offset */
} btree_node;


struct list {
	long offset;
	int level;
	struct list *next;
};

typedef struct list LIST;

LIST *Queue;
LIST *Q_last;

char command [MAX];		
char filename [MAX];
int  order; 		 	/* Order of B-tree */
long root; 				/* Offset of B-tree root node */ 
long node_offset;

/* Forward Declarations */
int  get_cmd( char * );
int  btree_search( int, btree_node * );
void write_to_file( btree_node *, long );
void read_root( );
void read_from_file( btree_node *, long );
void btree_insert( btree_node *, int );
void btree_print( );
void create_root( btree_node*, long );

/*
 ** Queue insert  
 */
void insert( long offset, int level ) {

	LIST *node;

	node = ( LIST * ) malloc( sizeof( LIST ) );
	node->next = NULL;
	node->offset = offset;
	node->level  = level; 
	
	if( Queue == NULL ) { 
		Queue = node;
		Q_last = node;
	}
	else  {
		Q_last->next = node;
		Q_last = node;
	}
}

/*
 ** Queue remove
 */
long remove_first(int *level ) {

	LIST *tmp = NULL;
	long off;

	tmp = Queue;
	Queue = Queue->next;
	if( Queue == NULL )
		Q_last = NULL;
	off = tmp->offset;
	*level = tmp->level;

	free( tmp );
	
	return off;
}


/*
 ** Compare function for qsort for index
 */
int compare( const void * a, const void * b ) {

   int *x = ( int * ) a;
   int *y = ( int * ) b;	

   return ( *x - *y );
}


/* 
 ** Get file size 
 */
long get_file_size() {

	FILE *fp;
	long size;

	fp = fopen( filename, "rb" );
	if( fp == NULL )
		return -1;
	fseek( fp, 0, SEEK_END );
	size = ftell(fp);
	fclose( fp );
	return size;
}


/*
 * Initialize or reset a btree node 
 */
void initialize( btree_node **node, int set ) {

	if( !set ) {
		*node = ( btree_node * ) malloc( sizeof( btree_node ) );
		(*node)->n = 0; 
		(*node)->key = (int *) calloc( order, sizeof( int ) ); 
		(*node)->child = (long *) calloc( order + 1, sizeof( long ) );
		(*node)->parent = 0;
	}
	else {
		(*node)->n = 0;
		(*node)->parent = 0;
		memset( (*node)->key, 0, order );
		memset( (*node)->child, 0, order + 1 );
	}
}


/*
 * Free the allocated B-tree node 
 */
void free_node( btree_node *node ) {

	free( node->key );
	free( node->child );
	free( node );

}


/* Main starts here */

int main( int argc, char **argv ) {

    char *token;
    int  key;
    int  rc;
	int  len, i;
	FILE *fp;
	btree_node *node = NULL;

    if( argc < 3 ) {
        printf( "assn_4 index-file order\n" );
        exit( -1 );
    }

	order = atoi( argv[2] );
	strcpy( filename, argv[1] );
	read_root( );
	

    while( 1 ) {

		memset( command, 0, MAX );
        rc = -1;

        fgets( command, MAX, stdin );

		len = strlen( command );
		if( command[len - 1] == '\n' )
			command[len - 1] = '\0';
 
        token = strtok( command, " " );
		if( token == NULL )
			continue;
        
        switch( get_cmd(token) ) {
            
            case ADD : if( (token = strtok( NULL, " " )) != NULL ) {
                	   	   key = atoi( token );
						   rc = 0;
						   initialize( &node, ALLOC );
						   read_from_file( node, root );
						   if( root != -1 && (btree_search( key, node) == 0) )
								printf( "Entry with key=%d already exists\n", key );
						   else 
						   		btree_insert( node, key );
 							
						   free_node( node );
                       }
					   
                       break;

        
		    case FIND : if( (token = strtok( NULL, " " )) != NULL ) {
                        	key = atoi( token );
                        	rc = 0;
							initialize( &node, ALLOC );
							read_from_file( node, root );
							if( root == -1 || (btree_search( key, node )) < 0 )
								printf( "Entry with key=%d does not exist\n", key );
							else
						    	printf( "Entry with key=%d exists\n", key ); 

 							free_node( node );
                        }
                        break;

			case PRINT : btree_print();
						 rc = 0;
						 break;

            case END : fp = fopen( filename, "r+b" );
					   if( fp != NULL ) {
					   	fwrite( &root, sizeof(long), 1, fp );
						fclose( fp );
					   }
					   rc = END;
					   break;

            default : printf( "Invalid request\n" ); 
					  break;
        }

    	if( rc == -1 ) {
        	printf( "Unable to process the command\n" );
        	exit( -1 );
    	}
	
		if( rc == END ) break;
		
		}	

    return 0;

}



/* 
 ** Check whether the command is valid
 */
int get_cmd( char *cmd ) {

	if( !strcmp( cmd, "add" ) )
		return 1;

	if( !strcmp( cmd, "find" ) )
        return 2;

	if( !strcmp( cmd, "print" ) )
        return 3;

	if( !strcmp( command, "end" ) )
        return 4;

	return 0;

}


/*
 ** Level order traversal of btree 
 */
void btree_print( ) {

	int i, n = 1, prev_n = 0; 
	long offset;
	btree_node *node;
	int level = 1;

	if( root == -1 ) {
		printf( " B-tree is empty\n" );
		return;
	}

	/* Insert the root offset into the Queue */
	insert( root , 1);
	initialize( &node , ALLOC );
	
	while( Queue != NULL ) {
		
		offset = remove_first(&level);
		read_from_file( node, offset );		
		
		for( i = 0; i < order; i++ ) {
			if( node->child[i] == 0 ) 
				break;
			insert( node->child[i], level+1 );	
		}

		if( n != prev_n ) {
			printf( " %d: ", n );
			prev_n = n;
		}

		for( i = 0; i < node->n; i++ ) {
			printf( "%d", node->key[i] );
			if( i < node->n - 1 )
				printf( "," );
		}
			
		if( !Queue ) {
			printf( " \n" );
			break;
		} 	
		else if( Queue->level != level ) {
			n++;
			printf( " \n" );
		}
		else 
			printf( " " );
	}

	free_node( node );
	Q_last = NULL;
}				
		
/*
 ** Search the index for the key
 */
int btree_search( int key, btree_node *node ) {

	int s = 0;
	
	while( s < node->n ) {
		
		if( key == node->key[ s ] ) 
			return 0;
		if( key < node->key[ s ] )
			break;
		s++;
	}

	if( node->child[ s ] != 0 ) {
		node_offset = node->child[ s ];
		read_from_file( node, node->child[ s ] ); 
		return btree_search( key, node );
	}			
	else
		return -1;
}	


/* 
 * B-tree insertion module 
 */
void btree_insert( btree_node *node, int key ) {

	btree_node *new_node = NULL, *tmp_node;
	int median, i = 0, j;
	long offset;

	/* If B-tree is empty */
	if( root == -1 ) {
		node->n = 1;
		node->key[0] = key;
		node->parent = -1;
		root = sizeof(long);
		node_offset = sizeof(long);
		write_to_file( node, sizeof(long) );
		return;
	}
	
    
	/* 
     * If Leaf node has enough space for the new key 
     */	
    else if( node->n < (order - 1) ) {	
		node->key[node->n] = key;
		node->n++;
		qsort( node->key, node->n, sizeof( int ), compare );
		write_to_file( node, node_offset );
		return;

	}


	/* Leaf node is full. split the node and promote the median to the parent.
	 * If parent is full iteratively split its parent.
	 */

	initialize( &new_node, ALLOC );
	initialize( &tmp_node, ALLOC );

    node->key[node->n] = key;
    node->n++;
    qsort( node->key, node->n, sizeof( int ), compare );

	do {
	
		if( node->parent == -1 ) {
			create_root( node, node_offset );
			break;
		}
		
		j = order/2;
		median = node->key[j];
		node->key[j] = 0;
		node->n = j;
		j++;
		
		new_node->parent = node->parent;
		offset = get_file_size();

		/* Fill the new node */
		for( i = 0; j<order; i++, j++ ) { 
			new_node->key[i] = node->key[j];
			new_node->n++;
			new_node->child[i] = node->child[j];
			if( new_node->child[i] != 0 ) {
				read_from_file( tmp_node, new_node->child[i] );
				tmp_node->parent = offset;
				write_to_file( tmp_node, new_node->child[i] );
			} 
			node->key[j] = 0;
			node->child[j] = 0;
		}
		new_node->child[i] = node->child[j];
		if( new_node->child[i] != 0 ) {
			read_from_file( tmp_node, new_node->child[i] );
			tmp_node->parent = offset;
			write_to_file( tmp_node, new_node->child[i] );
		} 	
		node->child[j] = 0;

		/* Write the new node to the file */
		
		write_to_file( new_node, offset );
		write_to_file( node, node_offset );	

		/* Read the parent node */
		node_offset = node->parent;
		read_from_file( node, node->parent );
		
		/* Add the promoted node and offset to the parent */	
		i = get_offset( node, median );
		node->key[node->n] = median;
   	 	node->n++;
    	qsort( node->key, node->n, sizeof( int ), compare );
		for( j = order; j > i; j-- )
			node->child[j] = node->child[j-1]; 

		node->child[i] = offset;
		initialize( &new_node, RESET );
		initialize( &tmp_node, RESET );
		
		if( node->n < order ) {
			write_to_file( node, node_offset );
			break;
		}

	}while( 1 );

	free_node( new_node );
	free_node( tmp_node );
	
		  
}

/* 
 * Find the appropriate position 
 */
int get_offset( btree_node *node, int median ) {

	int i;

	for( i = 0; i<node->n; i++ ) {
		if( node->key[i] > median )
			break;				
	}

	return i+1;
}


/*
 * Write the index and availability list to the file
 */

void write_to_file( btree_node *node, long offset ) {

	FILE *out; /* Output file stream */ 

	out = fopen( filename, "r+b" );
	if( out ==  NULL ) {
		printf( "File write failed\n"); 
		return;
	}

	if( root == -1 )
		fwrite( &root, sizeof( long ), 1, out );
	
	if( offset == -1 )
		fseek( out, 0, SEEK_END );
	else
		fseek( out, offset, SEEK_SET );

	fwrite( &node->n, sizeof( int ), 1, out );
	fwrite( node->key, sizeof( int ), order - 1, out );
	fwrite( node->child, sizeof( long ), order, out );
	fwrite( &node->parent, sizeof( long ), 1, out );
 
	fclose( out );
}


/*
 ** Handle the root node split 
 */
void create_root( btree_node *node, long node_offst ) {

	btree_node *root_node;
	btree_node *new_node, *tmp_node;
	long new_node_offset;
	long root_offset;
	int i, j;
	int median;

	initialize( &root_node, ALLOC );
	initialize( &new_node, ALLOC );
	initialize( &tmp_node, ALLOC );

	root_offset = get_file_size();
	
	new_node->parent = root_offset;
	node->parent = root_offset;
	node->n = order/2;
	j = order/2;
	median = node->key[j];
	node->key[j] = 0;
	j++;

	/* Fill the root */
	root_node->parent = -1;
	root_node->n = 1;
	root_node->key[0] = median;
	root_node->child[0] = node_offst;	
	
	write_to_file( root_node, root_offset );
	new_node_offset = get_file_size();

	/* Fill the new node */
	for( i = 0; j<order; i++, j++ ) { 
		new_node->key[i] = node->key[j];
		new_node->n++;
		new_node->child[i] = node->child[j];
		if( new_node->child[i] != 0 ) {
			read_from_file( tmp_node, new_node->child[i] );
			tmp_node->parent = new_node_offset;
			write_to_file( tmp_node, new_node->child[i] );
		} 
		node->key[j] = 0;
		node->child[j] = 0;
	}

	new_node->child[i] = node->child[j];	
	if( new_node->child[i] != 0 ) {
		read_from_file( tmp_node, new_node->child[i] );
		tmp_node->parent = new_node_offset;
		write_to_file( tmp_node, new_node->child[i] );
	}
	node->child[j] = 0;
	
	root_node->child[1] = new_node_offset;

	write_to_file( new_node, new_node_offset );
	write_to_file( root_node, root_offset );
	write_to_file( node, node_offst );

	root = root_offset;

	free_node( root_node );
	free_node( new_node );
	free_node( tmp_node );
}


/*
 * Read the root offset from index file 
 */
void read_root( ) {

    FILE *fp; /* Input file stream */ 

	fp = fopen( filename, "r+b" );  

	if ( fp == NULL ) { 
		root = -1; 
		fp = fopen( filename, "w+b" ); 
		fwrite( &root, sizeof( long ), 1, fp ); 
	} 
	else { 
		fread( &root, sizeof( long ), 1, fp ); 
		node_offset = root;
	} 

	fclose( fp );
}


/*
 * Read the node offset from the file
 */
void read_from_file( btree_node *node, long offset ) {

    FILE *fp; /* Input file stream */ 

	initialize( &node, RESET );

	if( root == -1 )
		return;

	fp = fopen( filename, "r+b" );  

	if ( fp == NULL ) {
		return; 
	} 
	else { 
		fseek( fp, offset, SEEK_SET );
		fread( &node->n, sizeof( int ), 1, fp );
		fread( node->key, sizeof( int ), order - 1, fp );
		fread( node->child, sizeof( long ), order, fp );
		fread( &node->parent, sizeof( long ), 1, fp );	
	} 

	fclose( fp );
}



