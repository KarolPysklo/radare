0x08048BA3 push ebp            
0x08048BA4 mov ebp, esp        
0x08048BA6 sub esp, 0x18       
0x08048BA9 mov eax, [ebp+0x18] 
0x08048BAC mov [ebp-0x4], eax  
0x08048BAF mov eax, [ebp-0x4]  
0x08048BB2 cmp eax, [ebp+0xc]  
0x08048BB5 jge dword 0xc75     
0x08048BBB mov byte [ebp-0xe], 0x0
0x08048BBF mov dword [ebp-0x8], 0x0
0x08048BC6 cmp dword [ebp-0x8], 0x7
0x08048BCA jg dword 0xc5b      
0x08048BD0 cmp dword [ebp+0x10], 0x0
0x08048BD4 jz 0xbeb            
0x08048BD6 sub esp, 0x4        
0x08048BD9 push 0x0            
0x08048BDB push dword [ebp-0x4]
0x08048BDE push dword [ebp+0x8]
0x08048BE1 call 0x5cc          
0x08048BE6 add esp, 0x10       
0x08048BE9 jmp 0xc02           
0x08048BEB sub esp, 0x4        
0x08048BEE push 0x2            
0x08048BF0 mov eax, [ebp-0x4]  
0x08048BF3 inc eax             
0x08048BF4 neg eax             
0x08048BF6 push eax            
0x08048BF7 push dword [ebp+0x8]
0x08048BFA call 0x5cc          
0x08048BFF add esp, 0x10       
0x08048C02 push dword [ebp+0x8]
0x08048C05 push 0x1            
0x08048C07 push 0x1            
0x08048C09 lea eax, [ebp-0xd]  
0x08048C0C push eax            
0x08048C0D call 0x64c          
0x08048C12 add esp, 0x10       
0x08048C15 movsx eax, byte [ebp-0xd]
0x08048C19 and eax, 0x1        
0x08048C1C mov [ebp-0xc], eax  
0x08048C1F cmp dword [ebp+0x14], 0x0
0x08048C23 jz 0xc3d            
0x08048C25 mov eax, 0x7        
0x08048C2A mov ecx, eax        
0x08048C2C sub ecx, [ebp-0x8]  
0x08048C2F mov eax, [ebp-0xc]  
0x08048C32 mov edx, eax        
0x08048C34 shl edx, cl         
0x08048C36 lea eax, [ebp-0xe]  
0x08048C39 or [eax], dl        
0x08048C3B jmp 0xc4c           
0x08048C3D mov ecx, [ebp-0x8]  
0x08048C40 mov eax, [ebp-0xc]  
0x08048C43 mov edx, eax        
0x08048C45 shl edx, cl         
0x08048C47 lea eax, [ebp-0xe]  
0x08048C4A or [eax], dl        
0x08048C4C lea eax, [ebp-0x8]  
0x08048C4F inc dword [eax]     
0x08048C51 lea eax, [ebp-0x4]  
0x08048C54 inc dword [eax]     
0x08048C56 jmp 0xbc6           
0x08048C5B sub esp, 0x8        
0x08048C5E movsx eax, byte [ebp-0xe]
0x08048C62 push eax            
0x08048C63 push dword 0x8049220
0x08048C68 call 0x61c          
0x08048C6D add esp, 0x10       
0x08048C70 jmp 0xbaf           
0x08048C75 leave               
0x08048C76 ret       
