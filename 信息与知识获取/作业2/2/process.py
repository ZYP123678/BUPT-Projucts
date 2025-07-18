import os
import re
import pickle
import time
import multiprocessing
import requests
from PIL import Image
from io import BytesIO
import base64
import pytesseract
from sklearn.feature_extraction.text import CountVectorizer
from collections import defaultdict
from concurrent.futures import ThreadPoolExecutor, as_completed
from tqdm import tqdm


article_path = r'C:\Codefield\info_know\2\bbc_articles\articles'
video_path = r'C:\Codefield\info_know\2\bbc_articles\videos'
photos_path = r'C:\Codefield\info_know\2\bbc_articles\photos'  # 新增: 图片目录

# 缓存文件路径
CACHE_DIR = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'cache')
BAG_CACHE = os.path.join(CACHE_DIR, 'bag_model.pkl')
COUNT_CACHE = os.path.join(CACHE_DIR, 'count_matrix.pkl')
INDEX_CACHE = os.path.join(CACHE_DIR, 'inverse_index.pkl')
METADATA_CACHE = os.path.join(CACHE_DIR, 'metadata.pkl')
URL_MAP_CACHE = os.path.join(CACHE_DIR, 'url_map.pkl')

# 图片和OCR相关缓存
IMAGE_CACHE_DIR = os.path.join(CACHE_DIR, 'images')
OCR_CACHE = os.path.join(CACHE_DIR, 'ocr_results.pkl')
PHOTO_METADATA_CACHE = os.path.join(CACHE_DIR, 'photo_metadata.pkl')

if not os.path.exists(CACHE_DIR):
    os.makedirs(CACHE_DIR)

if not os.path.exists(IMAGE_CACHE_DIR):
    os.makedirs(IMAGE_CACHE_DIR)

def get_file_paths():
    """获取所有文件路径，包括文章和视频"""
    paths = []
    
    # 获取文章文件
    if os.path.exists(article_path):
        article_files = [os.path.join(article_path, f) for f in os.listdir(article_path) if f.endswith('.txt')]
        paths.extend(article_files)
    
    # 获取视频文件
    if os.path.exists(video_path):
        video_files = [os.path.join(video_path, f) for f in os.listdir(video_path) if f.endswith('.txt')]
        paths.extend(video_files)
    
    return paths

def get_photo_paths():
    """获取所有图片文件路径"""
    photo_paths = []
    
    if os.path.exists(photos_path):
        photo_files = [os.path.join(photos_path, f) for f in os.listdir(photos_path) 
                      if f.lower().endswith(('.jpg', '.jpeg', '.png', '.gif'))]
        photo_paths.extend(photo_files)
    
    return photo_paths

def get_text_list():
    """从文件中读取内容并提取正文部分"""
    file_paths = get_file_paths()
    text_list = []
    
    for file_path in file_paths:
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
                
                content_match = re.search(r'内容:\s*([\s\S]*)', content)
                if content_match:
                    text_content = content_match.group(1).strip()
                    text_list.append(text_content)
                else:
                    text_list.append(content)
        except Exception as e:
            print(f"读取文件 {file_path} 时出错: {e}")
    
    return text_list

def perform_ocr(image_path):
    """对图片执行OCR识别"""
    try:
        # 检查OCR结果缓存
        cache_key = os.path.basename(image_path)
        ocr_cache = get_or_create_ocr_cache()
        
        if cache_key in ocr_cache:
            return ocr_cache[cache_key]
        
        img = Image.open(image_path)
        
        img = img.convert('L')  # 转为灰度
        
        text = pytesseract.image_to_string(img)
        result = text.strip()
        
        ocr_cache[cache_key] = result
        save_ocr_cache(ocr_cache)
        
        return result
    except Exception as e:
        print(f"OCR识别失败 ({image_path}): {e}")
        return ""

def get_or_create_ocr_cache():
    """获取或创建OCR结果缓存"""
    if os.path.exists(OCR_CACHE):
        try:
            with open(OCR_CACHE, 'rb') as f:
                return pickle.load(f)
        except Exception as e:
            print(f"加载OCR缓存失败: {e}")
    
    return {}

def save_ocr_cache(ocr_cache):
    """保存OCR结果缓存"""
    try:
        with open(OCR_CACHE, 'wb') as f:
            pickle.dump(ocr_cache, f)
    except Exception as e:
        print(f"保存OCR缓存失败: {e}")

def process_photos():
    """处理所有图片文件并提取OCR文本"""
    if os.path.exists(PHOTO_METADATA_CACHE):
        try:
            with open(PHOTO_METADATA_CACHE, 'rb') as f:
                return pickle.load(f)
        except Exception as e:
            print(f"加载图片元数据缓存失败: {e}")
    
    photo_paths = get_photo_paths()
    photo_metadata = []
    ocr_cache = get_or_create_ocr_cache()
    
    print(f"开始处理 {len(photo_paths)} 张图片的OCR文本...")
    
    # 并行处理图片OCR
    with ThreadPoolExecutor(max_workers=min(8, os.cpu_count() or 4)) as executor:
        future_to_photo = {}
        for photo_path in photo_paths:
            # 检查是否已在缓存中
            cache_key = os.path.basename(photo_path)
            if cache_key in ocr_cache:
                # 直接使用缓存结果
                photo_metadata.append({
                    'photo_path': photo_path,
                    'file_name': os.path.basename(photo_path),
                    'ocr_text': ocr_cache[cache_key]
                })
            else:
                future = executor.submit(perform_ocr, photo_path)
                future_to_photo[future] = photo_path
        
        for i, future in enumerate(as_completed(future_to_photo)):
            photo_path = future_to_photo[future]
            try:
                ocr_text = future.result()
                photo_metadata.append({
                    'photo_path': photo_path,
                    'file_name': os.path.basename(photo_path),
                    'ocr_text': ocr_text
                })
                
                if (i+1) % 10 == 0 or i+1 == len(future_to_photo):
                    print(f"已处理 {i+1}/{len(future_to_photo)} 张图片")
            except Exception as e:
                print(f"处理图片 {photo_path} 失败: {e}")
    
    try:
        with open(PHOTO_METADATA_CACHE, 'wb') as f:
            pickle.dump(photo_metadata, f)
    except Exception as e:
        print(f"保存图片元数据缓存失败: {e}")
    
    
    print(f"OCR识别示例 (前3张图片):")
    for item in photo_metadata[:3]:
        print(f"图片: {item['file_name']}")
        print(f"OCR文本: {item['ocr_text'][:100]}...")
        print("-"*50)
    return photo_metadata

def process_file_metadata(file_path):
    """处理单个文件的元数据 - 用于并行处理"""
    try:
        with open(file_path, 'r', encoding='utf-8') as f:
            content = f.read()
            
            title_match = re.search(r'标题: (.*?)(\n\n|$)', content)
            title = title_match.group(1) if title_match else os.path.basename(file_path)
            
            url_match = re.search(r'链接: (.*?)(\n\n|$)', content)
            
            # 如果找不到标准格式，尝试直接查找URL
            if not url_match or not url_match.group(1).strip():
                bbc_url_match = re.search(r'https?://www\.bbc\.co\.uk/news/[a-z0-9/-]+', content)
                if bbc_url_match:
                    url = bbc_url_match.group(0)
                else:
                    # 尝试查找任何URL
                    any_url_match = re.search(r'https?://\S+', content)
                    url = any_url_match.group(0) if any_url_match else ""
            else:
                url = url_match.group(1).strip()
            
            is_video = "视频" in content or "/videos/" in file_path
            
            # 从文章标题尝试匹配对应图片
            related_photos = []
            article_name = os.path.splitext(os.path.basename(file_path))[0]
            
            # 获取所有图片路径
            photo_paths = get_photo_paths()
            for photo_path in photo_paths:
                photo_name = os.path.splitext(os.path.basename(photo_path))[0]
                # 简单匹配：如果图片文件名包含文章文件名，则认为是相关图片
                if article_name.lower() in photo_name.lower() or photo_name.lower() in article_name.lower():
                    related_photos.append(photo_path)
            
            return {
                'file_path': file_path,
                'title': title,
                'url': url,
                'is_video': is_video,
                'related_photos': related_photos
            }
    except Exception as e:
        print(f"解析文件 {file_path} 元数据时出错: {e}")
        return None

def get_metadata():
    """获取每个文档的元数据（标题、URL等）- 使用并行处理"""
    if os.path.exists(METADATA_CACHE):
        try:
            with open(METADATA_CACHE, 'rb') as f:
                metadata = pickle.load(f)
                # 检查是否包含related_photos字段
                if metadata and 'related_photos' in metadata[0]:
                    return metadata
        except Exception as e:
            print(f"加载元数据缓存时出错: {e}")
    
    file_paths = get_file_paths()
    metadata = []
    
    print(f"开始处理 {len(file_paths)} 个文件的元数据...")
    
    # 使用线程池并行处理文件
    with ThreadPoolExecutor(max_workers=min(32, os.cpu_count() * 4)) as executor:
        future_to_file = {executor.submit(process_file_metadata, file_path): file_path for file_path in file_paths}
        for future in as_completed(future_to_file):
            result = future.result()
            if result:
                metadata.append(result)
    
    try:
        with open(METADATA_CACHE, 'wb') as f:
            pickle.dump(metadata, f)
    except Exception as e:
        print(f"保存元数据缓存时出错: {e}")
    
    return metadata

def get_text_list_with_ocr():
    """获取文本列表，包括OCR识别内容"""
    file_paths = get_file_paths()
    text_list = []
    
    photo_metadata = process_photos()
    photo_ocr_map = {item['photo_path']: item['ocr_text'] for item in photo_metadata}
    
    article_metadata = get_metadata()
    file_to_photos = {item['file_path']: item.get('related_photos', []) for item in article_metadata}
    
    for file_path in file_paths:
        try:
            with open(file_path, 'r', encoding='utf-8') as f:
                content = f.read()
                
                content_match = re.search(r'内容:\s*([\s\S]*)', content)
                text_content = content_match.group(1).strip() if content_match else content
                
                related_photos = file_to_photos.get(file_path, [])
                if related_photos:
                    ocr_texts = [photo_ocr_map.get(photo, "") for photo in related_photos]
                    ocr_texts = [text for text in ocr_texts if text.strip()]
                    if ocr_texts:
                        ocr_content = " ".join(ocr_texts)
                        text_content += f"\n[OCR_TEXT] {ocr_content}"
                
                text_list.append(text_content)
        except Exception as e:
            print(f"读取文件 {file_path} 时出错: {e}")
            text_list.append("") 
    
    return text_list

def get_bag(texts):
    """构建词袋模型 - 使用缓存"""
    if os.path.exists(BAG_CACHE) and os.path.exists(COUNT_CACHE):
        try:
            with open(BAG_CACHE, 'rb') as f:
                bag = pickle.load(f)
            with open(COUNT_CACHE, 'rb') as f:
                count = pickle.load(f)
            print("已从缓存加载词袋模型")
            return bag, count
        except Exception as e:
            print(f"加载词袋模型缓存时出错: {e}")
    
    print("构建新的词袋模型...")
    bag = CountVectorizer(
        token_pattern=r'\b\w+\b',        # 匹配单词边界的词
        stop_words='english',            # 去除英文停用词
        min_df=2,                        # 至少在2个文档中出现
        max_df=0.95                      # 在超过95%文档中出现的词被视为停用词
    )
    count = bag.fit_transform(texts)
    
    # 保存到缓存
    try:
        with open(BAG_CACHE, 'wb') as f:
            pickle.dump(bag, f)
        with open(COUNT_CACHE, 'wb') as f:
            pickle.dump(count, f)
    except Exception as e:
        print(f"保存词袋模型缓存时出错: {e}")
    
    return bag, count

def process_word_positions(args):
    """为单个词和文档查找位置 - 用于并行处理"""
    word, doc_idx, doc_text = args
    try:
        # 查找单词的所有出现位置
        positions = [m.span() for m in re.finditer(r'\b{}\b'.format(re.escape(word)), doc_text.lower())]
        if positions:  # 只有当找到位置时才返回
            return word, doc_idx, (doc_idx, len(positions), positions)
        return None
    except Exception as e:
        print(f"为单词 '{word}' 在文档 {doc_idx} 中查找位置时出错: {e}")
        return None

def generate_inverse_index(text_list, bag, count):
    """生成倒排索引 - 使用缓存和并行处理"""
    # 检查缓存
    if os.path.exists(INDEX_CACHE):
        try:
            with open(INDEX_CACHE, 'rb') as f:
                return pickle.load(f)
        except Exception as e:
            print(f"加载倒排索引缓存时出错: {e}")
    
    print("生成新的倒排索引...")
    start_time = time.time()
    result = defaultdict(dict)
    
    features = bag.get_feature_names_out()
    
    tasks = []
    for i, word in enumerate(features):
        for j in range(count.shape[0]):
            if count[j, i] > 0:
                tasks.append((word, j, text_list[j]))
    
    print(f"开始并行处理 {len(tasks)} 个索引任务...")
    processed = 0
    
    num_workers = min(os.cpu_count() or 4, 8)  # 最多8个进程
    
    with multiprocessing.Pool(processes=num_workers) as pool:
        for res in pool.imap_unordered(process_word_positions, tasks, chunksize=100):
            processed += 1
            if processed % 1000 == 0:
                print(f"已处理 {processed}/{len(tasks)} 任务 ({processed/len(tasks)*100:.1f}%)")
            
            if res:
                word, doc_idx, value = res
                result[word][doc_idx] = value
    
    print(f"倒排索引生成完成，用时 {time.time() - start_time:.2f} 秒")
    
    try:
        with open(INDEX_CACHE, 'wb') as f:
            pickle.dump(result, f)
    except Exception as e:
        print(f"保存倒排索引缓存时出错: {e}")
    
    return result

def get_photo_by_ocr_content(query):
    """根据OCR文本内容查找相关图片"""
    photo_metadata = process_photos()
    matching_photos = []
    
    query = query.lower()
    for photo in photo_metadata:
        ocr_text = photo.get('ocr_text', '').lower()
        if query in ocr_text:
            matching_photos.append(photo)
    
    matching_photos.sort(key=lambda x: -x['ocr_text'].lower().count(query))
    return matching_photos

def create_filepath_to_url_map():
    """创建文件路径到URL的映射，方便查询 - 使用缓存"""
    if os.path.exists(URL_MAP_CACHE):
        try:
            with open(URL_MAP_CACHE, 'rb') as f:
                return pickle.load(f)
        except Exception as e:
            print(f"加载URL映射缓存时出错: {e}")
    
    metadata_list = get_metadata()
    url_map = {}
    
    for item in metadata_list:
        file_path = item['file_path']
        url = item['url']
        if url and url.strip():
            url_map[file_path] = url
    
    try:
        with open(URL_MAP_CACHE, 'wb') as f:
            pickle.dump(url_map, f)
    except Exception as e:
        print(f"保存URL映射缓存时出错: {e}")
    
    return url_map

def create_photo_path_to_base64_map():
    """创建图片路径到base64编码的映射，用于前端显示"""
    photo_paths = get_photo_paths()
    photo_map = {}
    
    for path in photo_paths:
        try:
            with open(path, 'rb') as f:
                img_data = f.read()
                img_type = path.split('.')[-1].lower()
                if img_type == 'jpg' or img_type == 'jpeg':
                    mime_type = 'image/jpeg'
                elif img_type == 'png':
                    mime_type = 'image/png'
                elif img_type == 'gif':
                    mime_type = 'image/gif'
                else:
                    mime_type = 'image/jpeg'
                
                base64_data = base64.b64encode(img_data).decode('utf-8')
                photo_map[path] = f"data:{mime_type};base64,{base64_data}"
        except Exception as e:
            print(f"处理图片 {path} 失败: {e}")
    
    return photo_map

def clear_cache():
    """清除所有缓存文件"""
    cache_files = [
        BAG_CACHE, COUNT_CACHE, INDEX_CACHE, METADATA_CACHE, 
        URL_MAP_CACHE, OCR_CACHE, PHOTO_METADATA_CACHE
    ]
    
    for file in cache_files:
        if os.path.exists(file):
            try:
                os.remove(file)
                print(f"已删除缓存文件: {file}")
            except Exception as e:
                print(f"删除缓存文件 {file} 时出错: {e}")
    
    if os.path.exists(IMAGE_CACHE_DIR):
        for file in os.listdir(IMAGE_CACHE_DIR):
            try:
                os.remove(os.path.join(IMAGE_CACHE_DIR, file))
            except Exception as e:
                print(f"删除图片缓存文件 {file} 时出错: {e}")