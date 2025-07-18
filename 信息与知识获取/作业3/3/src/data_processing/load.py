import os
import re
import json
from pathlib import Path
from typing import List, Dict, Any, Union
from tqdm import tqdm
import pdfplumber
import logging


logging.basicConfig(level=logging.INFO, format='%(asctime)s - %(name)s - %(levelname)s - %(message)s')
logger = logging.getLogger(__name__)
class DataLoader:
    """法律文档数据加载器，用于处理TXT和PDF格式文件"""
    
    def __init__(self, data_dir: Union[str, Path]):
        """
        初始化数据加载器
        
        Args:
            data_dir: 数据目录路径
        """
        self.data_dir = Path(data_dir)
    
    def read_pdf_file(self, file_path: Union[str, Path]) -> Dict[str, Any]:
        """
        读取PDF文件并提取文本，同时删除换行符和多余空格
        
        Args:
            file_path: PDF文件路径
            
        Returns:
            包含处理后文件内容的字典
        """
        
        try:
            full_text = ""
            
            with pdfplumber.open(file_path) as pdf:
                # 提取所有页面文本
                for page in pdf.pages:
                    text = page.extract_text()
                    if text:
                        full_text += text + " "
            
            # 1. 替换所有换行符为空字符
            cleaned_text = full_text.replace('\n', '')
            # 2. 替换多个连续空格为单个空格
            cleaned_text = re.sub(r'\s+', ' ', cleaned_text)
            # 3. 替换中文间的空格（两个中文字符之间的空格）
            cleaned_text = re.sub(r'([\u4e00-\u9fa5])\s+([\u4e00-\u9fa5])', r'\1\2', cleaned_text)
            # 4. 去除首尾空格
            cleaned_text = cleaned_text.strip()
            
            return {
                'id': Path(file_path).stem,
                'filename': Path(file_path).name,
                'content': cleaned_text,
                'file_path': str(file_path),
                'language': 'chinese',
                'document_type': 'legal'
            }
        except Exception as e:
            logger.error(f"处理PDF文件出错 {file_path}: {e}")
            return {
                'id': Path(file_path).stem,
                'filename': Path(file_path).name,
                'content': f"文件处理错误: {str(e)}",
                'file_path': str(file_path),
                'document_type': 'legal',
                'error': str(e)
            }
    
    
    def load_all_documents(self) -> List[Dict[str, Any]]:
        """
        加载所有法律文档
        
        Returns:
            文档列表
        """
        results = []
        
        # 处理法律文档
        legal_dir = self.data_dir / "Legal"
        if legal_dir.exists():
            # 加载PDF文件
            pdf_files = list(legal_dir.glob("*.pdf"))
            for file_path in tqdm(pdf_files, desc="加载法律PDF文档"):
                doc = self.read_pdf_file(file_path)
                results.append(doc)
                
            if pdf_files:
                logger.info(f"已加载 {len(pdf_files)} 个法律PDF文档")
            
                
        else:
            logger.warning(f"法律文档目录不存在: {legal_dir}")
            
            # 尝试直接从数据目录加载
            pdf_files = list(self.data_dir.glob("*.pdf"))
            for file_path in tqdm(pdf_files, desc="从根目录加载PDF文档"):
                doc = self.read_pdf_file(file_path)
                results.append(doc)
            if pdf_files:
                logger.info(f"从根目录已加载 {len(pdf_files)} 个PDF文档")
            

        return results