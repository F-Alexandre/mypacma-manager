/*Este programa abre um pacote .pkg.tar.zst e extraie os arquivos
 *simulando o comportamento do pacman
 */
#include<archive.h>
#include<archive_entry.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<curl/curl.h>


//funcao principal para extrair o arquivo .pkg.tar.zst
int extrair_pacote(const char *caminho_pacote);

//funcao para auxiliar para escrever os dados recebidos da internet
//diretamente no arquivo em disco
size_t callback_escrita(void *ptr, size_t size , size_t nmemb, FILE *stream);

//funcao para baixar arquivos
int baixa_arquivo(cont char *url,const char *caminho_destino);

/*MAIN*/
int main(int argc , char **argv){


  if(argc < 2){
    printf("Uso do Pac:\n");
    printf(" %s -Syy  ",argv[0]);
    printf(" %s -U <pacote.zst> ",argv[0]);
  return 1;
  }


  if(strncmp(argv[1], "-Syy",10)==0){
    const char *url_db = "https://kernel.org";
    const char *destino = "core.db.tar.zst";
    return baixa_arquivo(url_db,destino);
  }else if(strncmp(argv[1], "-U",10)== 0){
      if(argc < 3){
        printf("Erro: Informe o caminho do pacote .pkg.tar.zst\n");
        return 1;
      }
       return extrair_pacote(argv[2]);

  }
    printf("[-] COMANDO INVALIDO");

    return 1;
}

/****/
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

/***/
size_t callback_escrita(void *ptr, size_t size , size_t nmemb, FILE *stream){

   size_t escrita = fwrite(ptr,size, nmemb, stream);
   return escrita;

}

/***/
int baixa_arquivo(cont char *url,const char *caminho_destino){

    CURL *curl;
    CURLcode res;
    FILE *arquivo;

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();

    if(!curl){
        fprintf(stderr, "[-]ERRO AO INICIALIZAR A LIBCURL.\n");
        return 1;
    }

    arquivo = fopen(caminho_destino, wb);

    if(!arquivo){
        fprintf(stderr, "[-]ERRO AO CRIAR O CAMINHO DE DESTINO: %s\n", caminho_destino);
        curl_easy_cleanup(curl);
        return 1;
    }

    printf("[*]INICIANDO DOWNLOAD......\n");
    printf("ORIGEM: %s\n",url);
    printf("DESTINO: %s\n" , caminho_destino);

    //configura as opcaoes da requisicao HTTP
    curl_easy_setopt(curl, CURLOPT,url);
    curl_easy_setopt(curl,CURLOPT_WRITEFUNCTION,callback_escrita());
    curl_easy_setopt(curl,CURLOPT_WRITEDATA, arquivo);


    //seguir redirecionamento HTTP
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION,1L);

    //define um agente de usuario para evitar bloqueios de servidores
    curl_easy_setopt(curl,CURLOPT_USERAGENT, "ProtoPac-Manager/0.1");

    //executa o download sincrona
    res= curl_easy_perform(curl);

    fclose(arquivo);

    if(res != CURLE_OK){
        fprintf(stderr,"[-] FALHA NO DOWNLOAD :(%s\n", curl_easy_strerror(res));
        //remove o arquivo corrompido
        remove(caminho_destino);
        curl_easy_cleanup(curl);
        curl_global_cleanup();
        return 1;
    }

    printf("[+]DOWNLOAD CONCLUIDO COM SUCESSO :)");

    curl_easy_cleanup(curl);
    curl_global_cleanup();

    return 0;
}





