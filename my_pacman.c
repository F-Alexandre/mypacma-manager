/*Este programa abre um pacote .pkg.tar.zst e extraie os arquivos
 *simulando o comportamento do pacman
 */
#include<archive.h>
#include<archive_entry.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>



int extrair_pacote(const char *caminho_pacote);

int main(int argc , char **argv){


    if(argc < 3 || strncmp(argv[1], "-U",1000) !=0)
    {
      printf("USO: %s -U <caminho_do_pacote.pkg.tar.zst>", argv[0]);
      return 1;
    }


 return extrair_pacote(argv[2]);

}

//funcao principal para extrair o arquivo .pkg.tar.zst
int extrair_pacote(const char *caminho_pacote){
  struct archive * a;
  struct archive *ext;
  struct archive_entry *entry;
  int flags;
  int r;


  //atributos que queremos preservar ao extrair (permissoes, donos do arquivos , etc)
  flags = ARCHIVE_EXTRACT_TIME;
  flags |= ARCHIVE_EXTRACT_PERM;
  flags |=ARCHIVE_EXTRACT_ACL;
  flags |=ARCHIVE_EXTRACT_FFLAGS;


   a= archive_read_new();
   ext =archive_write_disk_new();
   archive_write_disk_set_options(ext,flags);

   //habilitar o suporte a formatos tar e compactacao Zstandard(.zst)
   archive_read_support_format_tar(a);
   archive_read_support_filter_zstd(a);

   //abre o arquivo do pacote
   if((r= archive_read_open_filename(a, caminho_pacote,10240))){
    fprintf(stderr,"[-]Erro ao abrir o pacote %s\n",archive_error_string(a));
    return 1;
   }

   printf("[*] Lendo e extraindo o pacote: %s\n" ,caminho_pacote );


   //loop para ler cada arquivo de dentro do pacote compactado
   while(archive_read_next_header(a,&entry)==ARCHIVE_OK){
        const char * nome_arquivo = archive_entry_pathname(entry);

        //pula os arquivos de metadados internos do Arch

        if(strncmp(nome_arquivo,".PKGINFO",1000)==0 || strncmp(nome_arquivo,".INSTALL",1000)==0){
            archive_read_data_skip(a);
            continue;
        }

        printf("->> Extraindo: %s\n", nome_arquivo);

        //escreve o arquivo no disko

        r = archive_write_header(ext,entry);

        if(r < ARCHIVE_OK){
            fprintf(stderr,"[-] ERRO NO CABECALHO: %s\n",archive_error_string(ext));

        }else if(archive_entry_size(entry) >0){

            const void *buff;
            size_t size;
            int64_t offset;

            while((r = archive_read_data_block(a,&buff,&size , &offset))==ARCHIVE_OK){
                int r2 = archive_write_data_block(ext,buff,size,offset);
                if(r< ARCHIVE_OK){
                    fprintf(stderr, "[-]ERRO AO EXECUTAR DADOS: %s\n", archive_error_string(ext));
            return 1;
          }
        }
            if(r < ARCHIVE_WARN){
                fprintf(stderr,"[-] ERRO AO LER DADOS DO PACOTE: %s\n",archive_error_string(a));
            return 1;
            }


        }

        archive_write_finish_entry(ext);
   }

   archive_read_close(a);
   archive_read_free(a);
   archive_write_close(ext);
   archive_write_free(ext);

   printf("[+] Instalacao dos arquivos concluida com sucesso\n");
}


